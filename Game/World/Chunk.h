#pragma once
#include "Tile.h"
#include "../../Include/Vertex.h"
#include <vector>

struct Chunk {
    int chunkX, chunkY;
    std::vector<Tile> tiles;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    /**
     * Indicates whether this chunk needs its mesh regenerated.
     * Typically set when tile data changes.
     */
    bool needsUpdate = true;
    Chunk() = default;

    /**
     * @param x Chunk X coordinate in chunk space
     * @param y Chunk Y coordinate in chunk space
     * @param chunkSize Number of tiles per chunk edge (chunk is chunkSize² tiles)
     */
    Chunk(int x, int y, int chunkSize) : chunkX(x), chunkY(y) {
        tiles.resize(chunkSize * chunkSize);
    }
};