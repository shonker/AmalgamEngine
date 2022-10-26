#include "ServerConnectionSystem.h"
#include "World.h"
#include "Network.h"
#include "SpriteData.h"
#include "Name.h"
#include "PreviousPosition.h"
#include "Position.h"
#include "InputHistory.h"
#include "NeedsAdjacentChunks.h"
#include "Camera.h"
#include "Transforms.h"
#include "Config.h"
#include "UserConfig.h"
#include "Ignore.h"
#include "Log.h"

namespace AM
{
namespace Client
{
ServerConnectionSystem::ServerConnectionSystem(
    World& inWorld, EventDispatcher& inUiEventDispatcher, Network& inNetwork,
    SpriteData& inSpriteData, std::atomic<Uint32>& inCurrentTick)
: world{inWorld}
, network{inNetwork}
, spriteData{inSpriteData}
, connectionRequestQueue{inUiEventDispatcher}
, connectionResponseQueue{network.getEventDispatcher()}
, connectionErrorQueue{network.getEventDispatcher()}
, connectionState{ConnectionState::Disconnected}
, currentTick{inCurrentTick}
{
}

void ServerConnectionSystem::processConnectionEvents()
{
    if (connectionState == ConnectionState::Disconnected) {
        // TEMP
        // Kick off the server connection since we don't yet have a UI
        network.connect();
        connectionState = ConnectionState::AwaitingResponse;
        connectionAttemptTimer.updateSavedTime();
        LOG_INFO("SCS: Kicked off connection");
        // TEMP

        // Wait for a connection request from the UI.
        //ConnectionRequest connectionRequest;
        //if (connectionRequestQueue.pop(connectionRequest)) {
        //    if (Config::RUN_OFFLINE) {
        //        // No need to connect if we're running offline. Just mock up
        //        // the player data.
        //        initMockSimState();
        //        return;
        //    }
        //    else {
        //        // Kick off a connection attempt with the server.
        //        // Note: Eventually we'll instead send a ConnectionRequest to 
        //        //       the login server here with our login info.
        //        network.connect();
        //        connectionState = ConnectionState::AwaitingResponse;
        //        connectionAttemptTimer.updateSavedTime();
        //    }
        //}
    }
    else if (connectionState == ConnectionState::AwaitingResponse) {
        LOG_INFO("SCS: Awaiting response. Time: %.6f",
                 connectionAttemptTimer.getDeltaSeconds(false));
        // Wait for a connection response from the server.
        ConnectionResponse connectionResponse;
        if (connectionResponseQueue.pop(connectionResponse)) {
            initSimState(connectionResponse);
            connectionState = ConnectionState::Connected;
            LOG_INFO("SCS: Connected");
        }

        // If we've timed out, send a failure signal.
        if (connectionAttemptTimer.getDeltaSeconds(false)
            >= CONNECTION_RESPONSE_WAIT_S) {
            LOG_INFO("SCS: Calling disconnect 1");
            world.worldSignals.serverConnectionError.publish(
                {ConnectionError::Type::Failed});
            network.disconnect();
            connectionState = ConnectionState::Disconnected;
        }
    }

    // If the connection is lost, reset all network and sim state.
    ConnectionError connectionError;
    if (connectionErrorQueue.pop(connectionError)) {
        LOG_INFO("SCS: Calling disconnect 2");
        world.worldSignals.serverConnectionError.publish(
            {ConnectionError::Type::Disconnected});
        network.disconnect();
        clearSimState();
        connectionState = ConnectionState::Disconnected;
    }
}

void ServerConnectionSystem::initSimState(ConnectionResponse& connectionResponse)
{
    LOG_INFO(
        "Received connection response. ID: %u, tick: %u, pos: (%.4f, %.4f)",
        connectionResponse.entity, connectionResponse.tickNum,
        connectionResponse.x, connectionResponse.y);

    // Resize the world's tile map.
    world.tileMap.setMapSize(connectionResponse.mapXLengthChunks,
                             connectionResponse.mapYLengthChunks);
    world.worldSignals.tileMapExtentChanged.publish(
        world.tileMap.getTileExtent());
    LOG_INFO("Setting map size to: (%u, %u)ch.",
             connectionResponse.mapXLengthChunks,
             connectionResponse.mapYLengthChunks);

    // Aim our tick for some reasonable point ahead of the server.
    // The server will adjust us after the first message anyway.
    currentTick = connectionResponse.tickNum + Config::INITIAL_TICK_OFFSET;

    // Create the player entity using the ID we received.
    entt::entity playerEntity{connectionResponse.entity};
    entt::registry& registry{world.registry};
    entt::entity newEntity{registry.create(playerEntity)};
    if (newEntity != playerEntity) {
        LOG_FATAL("Created entity doesn't match received entity. Created: %u, "
                  "received: %u",
                  newEntity, playerEntity);
    }

    // Save the player entity ID for convenience.
    world.playerEntity = newEntity;

    // Set up the player's sim components.
    registry.emplace<Name>(newEntity,
                           std::to_string(static_cast<Uint32>(newEntity)));
    Position& playerPosition{registry.emplace<Position>(
        newEntity, connectionResponse.x, connectionResponse.y, 0.0f)};
    registry.emplace<PreviousPosition>(newEntity, connectionResponse.x,
                                       connectionResponse.y, 0.0f);
    registry.emplace<Velocity>(newEntity, 0.0f, 0.0f, 20.0f, 20.0f);
    registry.emplace<Input>(newEntity);

    // Set up the player's visual components.
    // TODO: Switch to logical screen size and do scaling in Renderer.
    UserConfig& userConfig{UserConfig::get()};
    Sprite& playerSprite{
        registry.emplace<Sprite>(newEntity, spriteData.get("roberto_0"))};
    registry.emplace<Camera>(newEntity, Camera::CenterOnEntity, Position{},
                             PreviousPosition{}, userConfig.getWindowSize());

    // Set up the player's bounding box, based on their sprite.
    registry.emplace<BoundingBox>(
        newEntity, Transforms::modelToWorldCentered(playerSprite.modelBounds,
                                                    playerPosition));

    // Set up the player's InputHistory component.
    registry.emplace<InputHistory>(newEntity);

    // Flag that we just moved and need to request all map data.
    registry.emplace<NeedsAdjacentChunks>(newEntity);
}

void ServerConnectionSystem::initMockSimState()
{
    // Create the player entity.
    entt::registry& registry{world.registry};
    entt::entity newEntity{registry.create()};

    // Save the player entity ID for convenience.
    world.playerEntity = newEntity;

    // Set up the player's sim components.
    registry.emplace<Name>(newEntity,
                           std::to_string(static_cast<Uint32>(newEntity)));
    Position& playerPosition{
        registry.emplace<Position>(newEntity, 0.0f, 0.0f, 0.0f)};
    registry.emplace<PreviousPosition>(newEntity, 0.0f, 0.0f, 0.0f);
    registry.emplace<Velocity>(newEntity, 0.0f, 0.0f, 20.0f, 20.0f);
    registry.emplace<Input>(newEntity);

    // Set up the player's visual components.
    // TODO: Switch to logical screen size and do scaling in Renderer.
    UserConfig& userConfig{UserConfig::get()};
    Sprite& playerSprite{
        registry.emplace<Sprite>(newEntity, spriteData.get("roberto_0"))};
    registry.emplace<Camera>(newEntity, Camera::CenterOnEntity, Position{},
                             PreviousPosition{}, userConfig.getWindowSize());

    // Set up the player's bounding box, based on their sprite.
    registry.emplace<BoundingBox>(
        newEntity, Transforms::modelToWorldCentered(playerSprite.modelBounds,
                                                    playerPosition));
    // TODO: Update our placement in the spatial partition.

    // Set up the player's InputHistory component.
    registry.emplace<InputHistory>(newEntity);
}

void ServerConnectionSystem::clearSimState()
{
    world.registry.clear();
    world.playerEntity = entt::null;
    world.tileMap.clear();
}

} // namespace Client
} // namespace AM
