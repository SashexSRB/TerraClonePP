#pragma once
#include "Tile.h"
#include "../../Engine/Vertex.h"
#include <vector>

struct Chunk {
    int chunkX, chunkY;
    std::vector<Tile> tiles;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool needsUpdate = true;

    Chunk() = default;

    Chunk(int x, int y, int chunkSize) : chunkX(x), chunkY(y) {
        tiles.resize(chunkSize * chunkSize);
    }
};