#include "Simulation.h"
#include "Network.h"
#include "SpriteData.h"
#include "EnttGroups.h"
#include "ISimulationExtension.h"
#include "ChunkUpdateSystem.h"
#include "TileUpdateSystem.h"
#include "EntityLifetimeSystem.h"
#include "PlayerInputSystem.h"
#include "PlayerMovementSystem.h"
#include "NpcMovementSystem.h"
#include "ComponentUpdateSystem.h"
#include "CameraSystem.h"
#include "Config.h"
#include "Log.h"
#include "entt/entity/registry.hpp"
#include <memory>
#include <string>

namespace AM
{
namespace Client
{
Simulation::Simulation(EventDispatcher& inUiEventDispatcher, Network& inNetwork,
                       SpriteData& inSpriteData)
: network{inNetwork}
, spriteData{inSpriteData}
, world{inSpriteData}
, currentTick{0}
, extension{nullptr}
, serverConnectionSystem{world, inUiEventDispatcher, network, inSpriteData,
                         currentTick}
, simulationStarted{serverConnectionSystem.simulationStartedSig}
, serverConnectionError{serverConnectionSystem.serverConnectionErrorSig}
{
    // Initialize our entt groups.
    EnttGroups::init(world.registry);

    // Register our current tick pointer with the classes that care.
    Log::registerCurrentTickPtr(&currentTick);
    network.registerCurrentTickPtr(&currentTick);

    // Initialize our systems to prepare them to run.
    initializeSystems();
}

Simulation::~Simulation() = default;

World& Simulation::getWorld()
{
    return world;
}

Uint32 Simulation::getCurrentTick()
{
    return currentTick;
}

Uint32 Simulation::getReplicationTick()
{
    return (currentTick + replicationTickOffset.get());
}

void Simulation::tick()
{
    // If we're online, process any server connection events.
    if (!Config::RUN_OFFLINE) {
        using ConnectionState = ServerConnectionSystem::ConnectionState;

        auto previousConnectionState{
            serverConnectionSystem.getConnectionState()};

        serverConnectionSystem.processConnectionEvents();

        auto currentConnectionState{
            serverConnectionSystem.getConnectionState()};

        // If we just lost connection, re-initialize our systems to prepare 
        // them for the next run.
        if ((previousConnectionState == ConnectionState::Connected)
            && (currentConnectionState == ConnectionState::Disconnected)) {
            initializeSystems();
        }

        // If we aren't connected, return early.
        if (serverConnectionSystem.getConnectionState()
            != ServerConnectionSystem::ConnectionState::Connected) {
            return;
        }
    }

    /* Calculate what tick we should be on. */
    // Increment the tick to the next.
    Uint32 targetTick{currentTick + 1};

    // If we're online, apply any adjustments that we receive from the
    // server.
    if (!Config::RUN_OFFLINE) {
        int adjustment{network.transferTickAdjustment()};
        if (adjustment != 0) {
            targetTick += adjustment;

            // Make sure NPC replication takes the adjustment into account.
            replicationTickOffset.applyAdjustment(adjustment);
        }
    }

    // Process ticks until we match what the server wants.
    // Note: This may cause us to not process any ticks, or to process multiple
    //       ticks.
    while (currentTick < targetTick) {
        /* Run all systems. */
        // Call the project's pre-everything logic.
        if (extension != nullptr) {
            extension->beforeAll();
        }

        // Process entities that need to be constructed or destructed.
        entityLifetimeSystem->processUpdates();

        // Process chunk updates from the server.
        chunkUpdateSystem->updateChunks();

        // Process tile updates from the server.
        tileUpdateSystem->updateTiles();

        // Call the project's pre-movement logic.
        if (extension != nullptr) {
            extension->afterMapAndConnectionUpdates();
        }

        // Process the held user input state and send change requests to the
        // server.
        // Note: Mouse and momentary inputs are processed through our OS event
        //       handling, prior to this tick.
        playerInputSystem->processHeldInputs();

        // Push the new input state into the player's history.
        playerInputSystem->addCurrentInputsToHistory();

        // Process player movement.
        playerMovementSystem->processMovement();

        // Process NPC movement.
        npcMovementSystem->updateNpcs();

        // Call the project's post-movement logic.
        if (extension != nullptr) {
            extension->afterMovement();
        }

        // Process component updates from the server.
        // Note: We do this last because the state that we receive is the 
        //       final state for this tick.
        componentUpdateSystem->processUpdates();

        // Move all cameras to their new positions.
        cameraSystem->moveCameras();

        // Call the project's post-everything logic.
        if (extension != nullptr) {
            extension->afterAll();
        }

        currentTick++;
    }
}

bool Simulation::handleOSEvent(SDL_Event& event)
{
    switch (event.type) {
        case SDL_MOUSEMOTION: {
            return false;
        }
        default: {
            // Default to assuming it's a momentary input.
            playerInputSystem->processMomentaryInput(event);
            return true;
        }
    }

    return false;
}

void Simulation::setExtension(std::unique_ptr<ISimulationExtension> inExtension)
{
    extension = std::move(inExtension);

    // Tell the project to initialize its systems.
    if (extension != nullptr) {
        extension->initializeSystems();
    }
}

void Simulation::initializeSystems()
{
    // Initialize our engine systems.
    chunkUpdateSystem = std::make_unique<ChunkUpdateSystem>(world, network);
    tileUpdateSystem = std::make_unique<TileUpdateSystem>(world, network);
    entityLifetimeSystem = std::make_unique<EntityLifetimeSystem>(
        *this, world, spriteData, network);
    playerInputSystem
        = std::make_unique<PlayerInputSystem>(*this, world, network);
    playerMovementSystem
        = std::make_unique<PlayerMovementSystem>(*this, world, network);
    npcMovementSystem
        = std::make_unique<NpcMovementSystem>(*this, world, network);
    componentUpdateSystem = std::make_unique<ComponentUpdateSystem>(
        *this, world, network, spriteData);
    cameraSystem = std::make_unique<CameraSystem>(world);

    // Tell the project to initialize its systems.
    if (extension != nullptr) {
        extension->initializeSystems();
    }
}

} // namespace Client
} // namespace AM
