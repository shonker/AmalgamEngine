#pragma once

#include "TileMapBase.h"
#include "SpriteData.h"

namespace AM
{
struct TileMapSnapshot;
class Tile;
struct TileSnapshot;
struct ChunkSnapshot;

namespace Server
{

/**
 * Owns and manages the world's tile map state.
 * Tiles are conceptually organized into 16x16 chunks.
 *
 * Persisted tile map data is loaded from TileMap.bin.
 *
 * Note: This class expects a TileMap.bin file to be present in the same
 *       directory as the application executable.
 */
class TileMap : public TileMapBase
{
public:
    /**
     * Attempts to parse TileMap.bin and construct the tile map.
     *
     * Errors if TileMap.bin doesn't exist or it fails to parse.
     */
    TileMap(SpriteData& inSpriteData);

    /**
     * Attempts to save the current tile map state to TileMap.bin.
     */
    ~TileMap();

    /**
     * Saves the map to a file with the given name, placed in the same
     * directory as the program binary.
     *
     * @param fileName  The file name to save to, with no path prepended.
     */
    void save(const std::string& fileName);

private:
    /**
     * Loads the given snapshot's data into this map.
     */
    void load(TileMapSnapshot& mapSnapshot);

    /**
     * Adds the sprite layers from the given tile to the given tile snapshot 
     * and the given chunk snapshot's palette.
     */
    void addTileLayersToSnapshot(const Tile& tile, TileSnapshot& tileSnapshot,
                                 ChunkSnapshot& chunkSnapshot);
};

} // End namespace Server
} // End namespace AM
