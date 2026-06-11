#include "ChunkManager.h"
#include "World.h"
#include <chrono>
#include <unordered_set>

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

    int w = world.getWidth(), h = world.getHeight();
    bool changed = false;

    // Build set of keys that should be loaded
    std::unordered_set<int64_t> wanted;
    for (int dx = -loadRadiusChunks; dx <= loadRadiusChunks; ++dx) {
        for (int dy = -loadRadiusChunks; dy <= loadRadiusChunks; ++dy) {
            int cx = pc.x + dx;
            int cy = pc.y + dy;
            if (cx < 0 || cy < 0 ||
                cx * chunkSize >= w || cy * chunkSize >= h) continue;
            wanted.insert(chunkKey(cx, cy));
        }
    }

    // Unload chunks no longer wanted
    lastRemovedKeys.clear();
    for (auto it = loadedChunks.begin(); it != loadedChunks.end(); ) {
        if (!wanted.count(it->first)) {
            lastRemovedKeys.push_back(it->first);
            it = loadedChunks.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    // Load only new chunks
    for (int64_t key : wanted) {
        if (loadedChunks.count(key)) continue;

        // Decode cx/cy from key
        int cx = static_cast<int>(key >> 32);
        int cy = static_cast<int>(key & 0xFFFFFFFF);

        Chunk c(cx, cy, chunkSize);
        for (int tx = 0; tx < chunkSize; ++tx) {
            for (int ty = 0; ty < chunkSize; ++ty) {
                int wx = cx * chunkSize + tx;
                int wy = cy * chunkSize + ty;
                if (wx >= w || wy >= h) continue;
                c.tiles[tx + ty * chunkSize] = world.getTile(wx, wy);
            }
        }
        loadedChunks[key] = std::move(c);
        changed = true;
    }

    return changed;
}













