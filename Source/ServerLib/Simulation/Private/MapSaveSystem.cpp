#include "MapSaveSystem.h"
#include "World.h"
#include "Config.h"

namespace AM
{
namespace Server
{

MapSaveSystem::MapSaveSystem(World& inWorld)
: world{inWorld}
, saveTimer{}
{
}

void MapSaveSystem::saveMapIfNecessary()
{
    // If enough time has passed, save the map state to TileMap.bin.
    if (saveTimer.getTime() >= Config::MAP_SAVE_PERIOD_S) {
        world.tileMap.save("TileMap.bin");

        saveTimer.reset();
    }
}

} // namespace Server
} // namespace AM
