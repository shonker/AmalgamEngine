#ifndef NETWORKOUTPUTSYSTEM_H
#define NETWORKOUTPUTSYSTEM_H

#include "Message_generated.h"
#include "SharedDefs.h"

namespace AM
{
namespace Server
{

class Game;
class World;
class Network;

/**
 *
 */
class NetworkOutputSystem
{
public:
    /** 30 game ticks per second. */
    static constexpr float NETWORK_OUTPUT_TICK_INTERVAL_S = 1 / 20.0f;

    NetworkOutputSystem(Game& inGame, World& inWorld, Network& inNetwork);

    /**
     * Sends dirty entity state data to all clients.
     */
    void updateClients(float deltaSeconds);

private:
    /**
     * Checks the world for dirty entities and relevant state information to all
     * connected clients.
     */
    void broadcastDirtyEntities();

    /**
     * Serializes the given entity's relevant world data.
     * @param entityID  The entity to serialize.
     * @return An offset where the data was stored in the builder.
     */
    flatbuffers::Offset<AM::fb::Entity> serializeEntity(EntityID entityID);

    Game& game;
    World& world;
    Network& network;

    static constexpr int BUILDER_BUFFER_SIZE = 512;
    flatbuffers::FlatBufferBuilder builder;

    /** The aggregated time since we last processed a tick. */
    float accumulatedTime;
};

} // namespace Server
} // namespace AM

#endif /* NETWORKOUTPUTSYSTEM_H */