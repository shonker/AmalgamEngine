#include "TileUpdateSystem.h"
#include "World.h"
#include "Network.h"
#include "AMAssert.h"

namespace AM
{
namespace Client
{
TileUpdateSystem::TileUpdateSystem(World& inWorld,
                                   EventDispatcher& inUiEventDispatcher,
                                   Network& inNetwork)
: world{inWorld}
, network{inNetwork}
, tileUpdateRequestQueue(inUiEventDispatcher)
, tileUpdateQueue(network.getEventDispatcher())
{
}

void TileUpdateSystem::updateTiles()
{
    // Process tile update requests from the UI.
    processUiRequests();

    // Process tile updates from the server.
    processNetworkUpdates();
}

void TileUpdateSystem::processUiRequests()
{
    // Process any waiting update requests from the UI.
    TileUpdateRequest uiRequest;
    while (tileUpdateRequestQueue.pop(uiRequest)) {
        // Send the request to the server.
        network.serializeAndSend<TileUpdateRequest>(uiRequest);
    }
}

void TileUpdateSystem::processNetworkUpdates()
{
    // Process any waiting tile updates from the server.
    TileUpdate tileUpdate;
    while (tileUpdateQueue.pop(tileUpdate)) {
        std::size_t updatedLayersIndex{0};

        // For each updated tile.
        for (TileUpdate::TileInfo& tileInfo : tileUpdate.tileInfo) {
            // Clear the tile (the message contains all of the tile's layers).
            world.tileMap.clearTile(tileInfo.tileX, tileInfo.tileY);

            // Fill the tile with the layers from the message.
            for (unsigned int layerIndex = 0; layerIndex < tileInfo.layerCount;
                 ++layerIndex) {
                AM_ASSERT(updatedLayersIndex < tileUpdate.updatedLayers.size(),
                          "Updated layers index is out of bounds.");

                int numericID{tileUpdate.updatedLayers[updatedLayersIndex]};
                world.tileMap.setTileSpriteLayer(tileInfo.tileX, tileInfo.tileY,
                                                 layerIndex, numericID);

                updatedLayersIndex++;
            }
        }
    }
}

} // End namespace Client
} // End namespace AM