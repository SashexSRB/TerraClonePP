#pragma once

#include "Chunk.h"
#include <unordered_map>
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

class World;

class ChunkManager {
public:
    ChunkManager(World& world, int chunkSize);

    bool update(int playerX, int playerY, int loadRadiusChunks);

    void markDirty(int worldX, int worldY);

    std::unordered_map<int64_t, Chunk>& getChunks() { return loadedChunks; }
    const std::unordered_map<int64_t, Chunk>& getChunks() const { return loadedChunks; }
    int getChunkSize() const { return chunkSize; }

    static int64_t chunkKey(int cx, int cy);
    glm::ivec2 playerChunk(int playerX, int playerY) const;

    static std::string meshKey(int cx, int cy);

private:
    World&     world;
    int        chunkSize;
    glm::ivec2 lastPlayerChunk = {-9999, -9999};
    std::unordered_map<int64_t, Chunk> loadedChunks;
};