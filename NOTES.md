Good plan—keeping it staged avoids breaking everything at once.

Below is a clean set of **new headers + source files** for the systems we discussed. They are designed to plug into your existing code with minimal immediate changes.

I’ll keep them **drop-in ready** and not force refactoring yet.

---

# 🟦 1. ChunkManager

## 📄 ChunkManager.h

```cpp
#pragma once

#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

class World;
struct Chunk;

class ChunkManager {
public:
    ChunkManager(World& world, int chunkSize);

    bool update(int playerX, int playerY, int loadRadiusChunks);

    glm::ivec2 getPlayerChunk(int playerX, int playerY) const;

    int64_t chunkKey(int x, int y) const;

    std::unordered_map<int64_t, Chunk>& getLoadedChunks();

private:
    World& world;
    int chunkSize;

    std::unordered_map<int64_t, Chunk> loadedChunks;

    glm::ivec2 lastPlayerChunk = {-9999, -9999};
};
```

---

## 📄 ChunkManager.cpp

```cpp
#include "ChunkManager.h"
#include "World.h"
#include "Chunk.h"

ChunkManager::ChunkManager(World& world, int chunkSize)
    : world(world), chunkSize(chunkSize) {}

int64_t ChunkManager::chunkKey(int x, int y) const {
    return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(y);
}

glm::ivec2 ChunkManager::getPlayerChunk(int playerX, int playerY) const {
    return { playerX / chunkSize, playerY / chunkSize };
}

std::unordered_map<int64_t, Chunk>& ChunkManager::getLoadedChunks() {
    return loadedChunks;
}

bool ChunkManager::update(int playerX, int playerY, int loadRadiusChunks) {
    glm::ivec2 playerChunk = getPlayerChunk(playerX, playerY);

    if (playerChunk == lastPlayerChunk)
        return false;

    lastPlayerChunk = playerChunk;

    std::unordered_map<int64_t, Chunk> newLoaded;
    bool changed = false;

    int width = world.getWidth();
    int height = world.getHeight();

    for (int dx = -loadRadiusChunks; dx <= loadRadiusChunks; ++dx) {
        for (int dy = -loadRadiusChunks; dy <= loadRadiusChunks; ++dy) {

            int cx = playerChunk.x + dx;
            int cy = playerChunk.y + dy;

            if (cx < 0 || cy < 0 || cx * chunkSize >= width || cy * chunkSize >= height)
                continue;

            int64_t key = chunkKey(cx, cy);

            auto& old = loadedChunks;

            if (old.count(key)) {
                newLoaded[key] = std::move(old[key]);
            } else {
                changed = true;

                Chunk c(cx, cy, chunkSize);

                for (int tx = 0; tx < chunkSize; ++tx) {
                    for (int ty = 0; ty < chunkSize; ++ty) {
                        int wx = cx * chunkSize + tx;
                        int wy = cy * chunkSize + ty;

                        if (wx >= width || wy >= height) continue;

                        c.tiles[tx + ty * chunkSize] = world.getTile(wx, wy);
                    }
                }

                newLoaded[key] = std::move(c);
            }
        }
    }

    if (newLoaded.size() != loadedChunks.size())
        changed = true;

    loadedChunks = std::move(newLoaded);
    return changed;
}
```

---

# 🟥 2. ChunkMesher (replaces generateChunkVertices)

## 📄 ChunkMesher.h

```cpp
#pragma once

#include <vector>
#include <cstdint>

struct Vertex;
struct Chunk;

class ChunkMesher {
public:
    void buildMesh(
        const Chunk& chunk,
        std::vector<Vertex>& vertices,
        std::vector<uint32_t>& indices,
        int chunkSize
    );
};
```

---

## 📄 ChunkMesher.cpp

```cpp
#include "ChunkMesher.h"
#include "Chunk.h"
#include "../Rendering/MeshUtils.h"
#include "../Constants.h"
#include "../Items/Item.h"

void ChunkMesher::buildMesh(
    const Chunk& chunk,
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices,
    int chunkSize)
{
    std::vector<QuadSpec> quads;

    for (int tx = 0; tx < chunkSize; ++tx) {
        for (int ty = 0; ty < chunkSize; ++ty) {

            const Tile& tile = chunk.tiles[tx + ty * chunkSize];

            if (!tile.isActive && tile.wallId == 0)
                continue;

            float wx = (chunk.chunkX * chunkSize + tx) * Constants::TileSize;
            float wy = (chunk.chunkY * chunkSize + ty) * Constants::TileSize;

            if (tile.wallId != 0) {
                const GameItem& item = Registry::getWall(tile.wallId);

                quads.push_back({
                    wx, wy,
                    Constants::TileSize, Constants::TileSize,
                    item.zValue,
                    getTexCoords(item.texX, item.texY,
                                  Constants::AtlasWidth,
                                  Constants::AtlasTileSize)
                });
            }

            if (tile.isActive) {
                const GameItem& item = Registry::get(tile.tileId);

                quads.push_back({
                    wx, wy,
                    Constants::TileSize, Constants::TileSize,
                    item.zValue,
                    getTexCoords(item.texX, item.texY,
                                  Constants::AtlasWidth,
                                  Constants::AtlasTileSize)
                });
            }
        }
    }

    buildMesh(vertices, indices, quads);
}
```

---

# 🟪 3. WorldSerializer (save/load moved out of World)

## 📄 WorldSerializer.h

```cpp
#pragma once

#include <string>

class World;
class Inventory;

class WorldSerializer {
public:
    static void save(const World& world,
                     const std::string& path,
                     bool binary,
                     const Inventory& inventory);

    static bool load(World& world,
                     const std::string& path,
                     bool binary,
                     Inventory& inventory);
};
```

---

## 📄 WorldSerializer.cpp

```cpp
#include "WorldSerializer.h"
#include "World.h"
#include "../UI/Inventory.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

// NOTE: This is a direct move of your existing logic.
// No behavior changes yet.

void WorldSerializer::save(
    const World& world,
    const std::string& path,
    bool binary,
    const Inventory& inventory)
{
    fs::create_directories(fs::path(path).parent_path());

    std::ofstream f;

    if (binary)
        f.open(path, std::ios::binary);
    else
        f.open(path);

    if (!f)
        throw std::runtime_error("Failed to open save file");

    // ---- COPY YOUR CURRENT SAVE LOGIC HERE (unchanged) ----
    // (we will clean it later after refactor is stable)
}

bool WorldSerializer::load(
    World& world,
    const std::string& path,
    bool binary,
    Inventory& inventory)
{
    if (!fs::exists(path))
        return false;

    std::ifstream f;

    if (binary)
        f.open(path, std::ios::binary);
    else
        f.open(path);

    if (!f)
        return false;

    // ---- COPY YOUR CURRENT LOAD LOGIC HERE (unchanged) ----

    return true;
}
```

---

# 🟡 Important note

I intentionally did NOT:

* change logic
* optimize anything
* rewrite algorithms

This is a **pure structural extraction phase**.

---

# 📦 After you add these

You will temporarily have:

* duplicated functionality (World still has old methods)
* no broken behavior
* easy rollback if needed

---

# 👉 Next step (when you say “good”)

We will do:

### Phase 2: safe wiring changes

* Replace `World::updateChunks` → `ChunkManager`
* Replace `World::generateChunkVertices` → `ChunkMesher`
* Replace `World::save/load` → `WorldSerializer`
* Keep API identical so nothing breaks externally

---

If you want, next I can also show you a **clean updated World.h after extraction** so you can see the end goal before touching anything.
