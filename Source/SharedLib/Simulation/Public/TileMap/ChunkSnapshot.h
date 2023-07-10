#pragma once

#include "TileLayers.h"
#include "TileSnapshot.h"
#include "SharedConfig.h"
#include <vector>
#include <array>
#include <string>

namespace AM
{

/**
 * Holds chunk data in a persistable form (palette IDs instead of pointers).
 *
 * Used in saving/loading the tile map.
 */
struct ChunkSnapshot {
    struct PaletteEntry
    {
        /** The type of tile layer that this entry represents. */
        TileLayer::Type layerType{TileLayer::Type::None};

        /** The string ID of the sprite set that this entry refers to. */
        std::string spriteSetID{""};

        /** The index within spriteSet.sprites that this entry refers to.
            For Walls, cast this to Wall::Type. For Floor Coverings and Objects,
            cast this to Rotation::Direction. For Floors, this will always be 0
            (floor sprite sets only have 1 sprite). */
        Uint8 spriteIndex{0};
    };

    /** Used as a "we should never hit this" cap on the number of entries in a
        palette. Only checked in debug builds. */
    static constexpr std::size_t MAX_PALETTE_ENTRIES{1000};

    /** Used as a "we should never hit this" cap on the size of each ID string
        in the palette. Only checked in debug builds. */
    static constexpr std::size_t MAX_ID_LENGTH{50};

    /** Holds an entry for each sprite used in this chunk's tiles. Part of a 
        space-saving approach that lets TileSnapshot hold indices into this 
        palette instead of directly holding the data. */
    std::vector<PaletteEntry> palette;

    /** The tiles that make up this chunk, stored in row-major order. */
    std::array<TileSnapshot, SharedConfig::CHUNK_TILE_COUNT> tiles;

    /**
     * Returns the palette index for the given palette entry info.
     * If the palette doesn't have a matching entry, it will be added.
     */
    std::size_t getPaletteIndex(TileLayer::Type tileLayerType,
                                const std::string& spriteSetID,
                                Uint8 spriteIndex)
    {
        // TODO: If this gets to be a performance issue, we can look into
        //       switching palette to a map. Serialization will be more
        //       complicated, though.
        // Check if we already have this ID.
        for (std::size_t i = 0; i < palette.size(); ++i) {
            if ((palette[i].layerType == tileLayerType)
                && (palette[i].spriteSetID == spriteSetID)
                && (palette[i].spriteIndex == spriteIndex)) {
                // We already have the string, returns its index.
                return i;
            }
        }

        // We didn't have a matching entry, add it.
        if (palette.size() < UINT8_MAX) {
            palette.emplace_back(tileLayerType, spriteSetID, spriteIndex);
        }
        else {
            // TODO: If this becomes an issue, either switch to Uint16 or 
            //       find some more efficient way to grow the space.
            LOG_FATAL("Ran out of palette slots.");
        }
        return (palette.size() - 1);
    }
};

template<typename S>
void serialize(S& serializer, ChunkSnapshot::PaletteEntry& paletteEntry)
{
    serializer.value1b(paletteEntry.layerType);
    serializer.text1b(paletteEntry.spriteSetID, ChunkSnapshot::MAX_ID_LENGTH);
    serializer.value1b(paletteEntry.spriteIndex);
}

template<typename S>
void serialize(S& serializer, ChunkSnapshot& chunkSnapshot)
{
    serializer.container(chunkSnapshot.palette, ChunkSnapshot::MAX_PALETTE_ENTRIES);
    serializer.container(chunkSnapshot.tiles);
}

} // End namespace AM
