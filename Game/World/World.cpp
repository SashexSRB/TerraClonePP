#include "World.h"

#include <cstring>

#include "../Constants.h"
#include "../Items/Item.h"
#include "../Rendering/MeshUtils.h"
#include "../../Lib/PerlinNoise.hpp"

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>

World::World(int w, int h) : chunks(*this, 64), width(w), height(h) {
    tiles.resize(w, std::vector<Tile>(h));
}

namespace fs = std::filesystem;

Tile &World::getTile(int x, int y) {
    return tiles[x][y];
}

const Tile &World::getTile(int x, int y) const {
    return tiles[x][y];
}

void World::setTile(int x, int y, Tile t) {
    tiles[x][y] = t;
    chunks.markDirty(x, y);

    // sync into loaded chunk copy
    int cx = x / chunks.getChunkSize();
    int cy = y / chunks.getChunkSize();
    int64_t key = chunks.chunkKey(cx, cy);

    auto& loaded = chunks.getChunks();
    auto it = loaded.find(key);

    if (it != loaded.end()) {
        int lx = x % chunks.getChunkSize();
        int ly = y % chunks.getChunkSize();

        it->second.tiles[lx + ly * chunks.getChunkSize()] = t;
    }
}

void World::generate(unsigned int s) {
    seed = s;
    Generator generator(s);
    generator.generate(*this);
}