#include "World.h"
#include "Generator.h"

World::World(int w, int h) : chunks(*this, 64), width(w), height(h) {
    tiles.resize(w * h);
}

Tile &World::getTile(int x, int y) {
    return tiles[x + y * width];
}

const Tile &World::getTile(int x, int y) const {
    return tiles[x + y * width];
}

void World::setTile(int x, int y, Tile t) {
    tiles[x + y * width] = t;
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