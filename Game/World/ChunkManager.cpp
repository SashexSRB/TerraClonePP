#include "ChunkManager.h"

#include <chrono>

#include "World.h"

ChunkManager::ChunkManager(World& world, int chunkSize) : world(world), chunkSize(chunkSize) {}

int64_t ChunkManager::chunkKey(int cx, int cy) {
    return (static_cast<int64_t>(cx) << 32 | static_cast<int32_t>(cy));
}

glm::ivec2 ChunkManager::playerChunk(int playerX, int playerY) const {
    return { playerX / chunkSize, playerY / chunkSize };
}

std::string ChunkManager::meshKey(int cx, int cy) {
    return "chunk_" + std::to_string(cx) + "_" + std::to_string(cy);
}

void ChunkManager::markDirty(int worldX, int worldY) {
    int cx = worldX / chunkSize;
    int cy = worldY / chunkSize;

    int64_t key = chunkKey(cx, cy);

    auto it = loadedChunks.find(key);
    if (it != loadedChunks.end())
        it ->second.needsUpdate = true;
}

bool ChunkManager::update(int playerX, int playerY, int loadRadiusChunks) {
    glm::ivec2 pc = playerChunk(playerX, playerY);
    if (pc == lastPlayerChunk) return false;
    lastPlayerChunk = pc;

    std::unordered_map<int64_t, Chunk> newLoaded;
    bool changed = false;
    int w = world.getWidth(), h = world.getHeight();

    for (int dx = -loadRadiusChunks; dx <= loadRadiusChunks; ++dx) {
        for (int dy = -loadRadiusChunks; dy <= loadRadiusChunks; ++dy) {
            int cx = pc.x + dx;
            int cy = pc.y + dy;

            if (cx < 0 || cy < 0 || cx * chunkSize >= w || cy * chunkSize >= h) continue;

            int64_t key = chunkKey(cx, cy);
            if (loadedChunks.count(key)) {
                newLoaded[key] = std::move(loadedChunks[key]);
            } else {
                changed = true;
                Chunk c(cx, cy, chunkSize);
                for (int tx = 0; tx < chunkSize; ++tx) {
                    for (int ty = 0; ty < chunkSize; ++ty) {
                        int wx = cx * chunkSize + tx;
                        int wy = cy * chunkSize + ty;

                        if (wx >= w || wy >= h) continue;
                        c.tiles[tx + ty * chunkSize] = world.getTile(wx, wy);
                    }
                }
                newLoaded[key] = std::move(c);
            }
        }
    }

    if (newLoaded.size() != loadedChunks.size()) changed = true;

    loadedChunks = std::move(newLoaded);
    return changed;
}













