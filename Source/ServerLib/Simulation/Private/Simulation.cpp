#include "Simulation.h"
#include "Network.h"
#include "EnttGroups.h"
#include "ISimulationExtension.h"
#include "InteractionRequest.h"
#include "Interaction.h"
#include "SystemMessage.h"
#include "Log.h"
#include "Timer.h"
#include "tracy/Tracy.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"

namespace AM
{
namespace Server
{
Simulation::Simulation(Network& inNetwork, SpriteData& inSpriteData)
: network{inNetwork}
, lua{std::make_unique<sol::state>()}
, world{inSpriteData, *lua}
, currentTick{0}
, engineLuaBindings{*lua, world}
, extension{nullptr}
, interactionRequestQueue{inNetwork.getEventDispatcher()}
, interactionQueueMap{}
, clientConnectionSystem{*this, world, network, inSpriteData}
, nceLifetimeSystem{world, network}
, componentChangeSystem{world, network, inSpriteData}
, tileUpdateSystem{world, network}
, inputSystem{*this, world, network}
, movementSystem{world}
, aiSystem{world}
, clientAOISystem{*this, world, network}
, movementSyncSystem{*this, world, network}
, componentSyncSystem{*this, world, network, inSpriteData}
, chunkStreamingSystem{world, network}
, scriptDataSystem{world, network}
, mapSaveSystem{world}
{
    // Initialize our entt groups.
    EnttGroups::init(world.registry);

    // Initialize the Lua engine and add our bindings.
    lua->open_libraries(sol::lib::base);
    engineLuaBindings.addBindings();

    // We use "errorString" to hold strings to send back to the user if one 
    // of our bound functions encountered an error.
    (*lua)["errorString"] = "";

    // Register our current tick pointer with the classes that care.
    Log::registerCurrentTickPtr(&currentTick);
    network.registerCurrentTickPtr(&currentTick);
}

Simulation::~Simulation() = default;

void Simulation::registerInteractionQueue(Uint8 interactionType,
                                          std::queue<InteractionRequest>& queue)
{
    if (interactionQueueMap[interactionType] != nullptr) {
        LOG_FATAL("Only one queue can be registered for each interaction type.");
    }

    interactionQueueMap[interactionType] = &queue;
}

World& Simulation::getWorld()
{
    return world;
}

sol::state& Simulation::getLua()
{
    return *lua;
}

Uint32 Simulation::getCurrentTick()
{
    return currentTick;
}

void Simulation::tick()
{
    ZoneScoped;

    /* Run all systems. */
    // Call the project's pre-everything logic.
    if (extension != nullptr) {
        extension->beforeAll();
    }

    // Process client connections and disconnections.
    clientConnectionSystem.processConnectionEvents();

    // Process requests to create or destroy non-client-controlled entities.
    nceLifetimeSystem.processUpdateRequests();

    // Process requests to change components.
    componentChangeSystem.processChangeRequests();

    // Receive and process tile update requests.
    tileUpdateSystem.updateTiles();

    // Call the project's pre-movement logic.
    if (extension != nullptr) {
        extension->afterMapAndConnectionUpdates();
    }

    // Push any waiting interaction messages into the system queues.
    dispatchInteractionMessages();

    // Send updated tile state to nearby clients.
    tileUpdateSystem.sendTileUpdates();

    // Receive and process client input messages.
    inputSystem.processInputMessages();

    // Move all of our entities.
    movementSystem.processMovements();
    
    // Run all of our AI.
    aiSystem.processAITick();

    // Call the project's post-movement logic.
    if (extension != nullptr) {
        extension->afterMovement();
    }

    // Update each client entity's "entities in my AOI" list and send Init/
    // Delete messages.
    clientAOISystem.updateAOILists();

    // Send any updated entity movement state to nearby clients.
    movementSyncSystem.sendMovementUpdates();

    // Send any remaining updated entity component state to nearby clients.
    componentSyncSystem.sendUpdates();

    // Call the project's post-movement-sync logic.
    if (extension != nullptr) {
        extension->afterClientSync();
    }

    // Respond to chunk data requests.
    chunkStreamingSystem.sendChunks();

    // Respond to script data requests.
    scriptDataSystem.sendScripts();

    // If enough time has passed, save the world's tile map state.
    mapSaveSystem.saveMapIfNecessary();

    // Call the project's post-everything logic.
    if (extension != nullptr) {
        extension->afterAll();
    }

    currentTick++;

    FrameMark;
}

void Simulation::setExtension(std::unique_ptr<ISimulationExtension> inExtension)
{
    extension = std::move(inExtension);
    nceLifetimeSystem.setExtension(extension.get());
    tileUpdateSystem.setExtension(extension.get());
}

void Simulation::dispatchInteractionMessages()
{
    entt::registry& registry{world.registry};

    // Dispatch any waiting interaction requests.
    InteractionRequest interactionRequest{};
    while (interactionRequestQueue.pop(interactionRequest)) {
        entt::entity clientEntity{interactionRequest.clientEntity};
        entt::entity targetEntity{interactionRequest.targetEntity};

        // Check that the client and target both exist.
        if (!(world.entityIDIsInUse(clientEntity))
            || !(world.entityIDIsInUse(targetEntity))) {
            continue;
        }

        // Check that the client is in range of the target. 
        const Position& clientPosition{registry.get<Position>(clientEntity)};
        const Position& targetPosition{registry.get<Position>(targetEntity)};
        if (clientPosition.squaredDistanceTo(targetPosition)
            > (SharedConfig::SQUARED_INTERACTION_DISTANCE)) {
            network.serializeAndSend(
                interactionRequest.netID,
                SystemMessage{"You must move closer to interact with that."});
            continue;
        }

        // Check that the target actually has this interaction type.
        if (auto interactionPtr = registry.try_get<Interaction>(targetEntity);
            (interactionPtr == nullptr)
            || !interactionPtr->contains(interactionRequest.interactionType)) {
            continue;
        }

        // Dispatch the interaction.
        Uint8 interactionType{interactionRequest.interactionType};
        if (interactionQueueMap[interactionType] != nullptr) {
            interactionQueueMap[interactionType]->push(interactionRequest);
        }
    }
}

} // namespace Server
} // namespace AM
