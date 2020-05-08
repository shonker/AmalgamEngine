#include "Game.h"
#include "Network.h"
#include "Debug.h"

namespace AM
{
namespace Server
{

Game::Game(Network& inNetwork)
: world()
, network(inNetwork)
, networkInputSystem(world, network)
, movementSystem(world)
, networkOutputSystem(*this, world, network)
, builder(BUILDER_BUFFER_SIZE)
, accumulatedTime(0.0f)
, currentTick(0)
{
    world.setSpawnPoint({64, 64});
    Debug::registerCurrentTickPtr(&currentTick);
}

void Game::tick(float deltaSeconds)
{
    accumulatedTime += deltaSeconds;

    // Process as many game ticks as have accumulated.
    while (accumulatedTime >= GAME_TICK_INTERVAL_S) {
        currentTick++;

        // Add any new connections.
        std::shared_ptr<Peer> newClient = network.getNewClient();
        while (newClient != nullptr) {
            // Build their entity.
            EntityID newID = world.addEntity("Player");
            const Position& spawnPoint = world.getSpawnPoint();
            world.positions[newID].x = spawnPoint.x;
            world.positions[newID].y = spawnPoint.y;
            world.movements[newID].maxVelX = 250;
            world.movements[newID].maxVelY = 250;
            world.attachComponent(newID, ComponentFlag::Input);
            world.attachComponent(newID, ComponentFlag::Movement);
            world.attachComponent(newID, ComponentFlag::Position);
            world.attachComponent(newID, ComponentFlag::Sprite);

            // Add them to the network's map.
            network.addClient(newID, newClient);

            // Prep the builder for a new message.
            builder.Clear();

            // Send them their ID and spawn point.
            auto response = fb::CreateConnectionResponse(builder, currentTick, newID,
                spawnPoint.x, spawnPoint.y);
            auto encodedMessage = fb::CreateMessage(builder,
                fb::MessageContent::ConnectionResponse, response.Union());
            builder.Finish(encodedMessage);

            Uint8* buffer = builder.GetBufferPointer();
            BinaryBufferSharedPtr message = std::make_shared<std::vector<Uint8>>(
            buffer, (buffer + builder.GetSize()));

            bool result = network.send(newClient, message);
            if (!result) {
                std::cerr << "Failed to send response." << std::endl;
            }

            newClient = network.getNewClient();
        }

        // Run all systems.
        networkInputSystem.processInputEvents();

        movementSystem.processMovements(GAME_TICK_INTERVAL_S);

        networkOutputSystem.updateClients(GAME_TICK_INTERVAL_S);

        accumulatedTime -= GAME_TICK_INTERVAL_S;
    }
}

Uint32 Game::getCurrentTick()
{
    return currentTick;
}

} // namespace Server
} // namespace AM