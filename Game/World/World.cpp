#include "World.h"

#include <cstring>

#include "../Constants.h"
#include "../Rendering/MeshUtils.h"
#include "../../Lib/PerlinNoise.hpp"

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::unordered_map<uint16_t, TileProperties> TileRegistry::tileTypes;
std::unordered_map<uint16_t, TileProperties> TileRegistry::wallTypes;

int64_t World::chunkKey(int x, int y) {
    return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(y);
}

glm::ivec2 World::getPlayerChunk(int playerX, int playerY) const {
    return {playerX / chunkSize, playerY / chunkSize};
}

void TileRegistry::initialize() {
    static const std::vector<TileDefinition> tileDefs = {
        {0, "Air",       0,0, false, 0.0f},
        {1, "Dirt",      1,0, true,  0.2f},
        {2, "Stone",     2,0, true,  0.2f},
        {3, "Grass",     3,0, true,  0.2f},
        {4, "Sand",      4,0, true,  0.2f},
        {5, "Sandstone", 5,0, true,  0.2f},
        {6, "Snow",      6,0, true,  0.2f},
        {7, "Ice",       7,0, true,  0.2f},
    };

    static const std::vector<TileDefinition> wallDefs = {
        {0, "None",      0,1, false, 1.0f},
        {1, "StoneWall", 1,1, false, 1.0f},
    };

    for (const auto &def: tileDefs) {
        tileTypes[def.id] = {
            def.name, {def.texX, def.texY}, def.isSolid, def.zValue
        };
    }

    for (const auto &def: wallDefs) {
        wallTypes[def.id] = {
            def.name,
            {def.texX, def.texY},
            def.isSolid,
            def.zValue,
        };
    }
}

World::World(int w, int h) : width(w), height(h) {
    tiles.resize(w, std::vector<Tile>(h));
}

Tile &World::getTile(int x, int y) {
    return tiles[x][y];
}

void World::setTile(int x, int y, Tile t) {
    tiles[x][y] = t;

    int cx = x / chunkSize;
    int cy = y / chunkSize;
    int64_t key = chunkKey(cx, cy);

    if (loadedChunks.count(key)) {
        int lx = x % chunkSize;
        int ly = y % chunkSize;
        loadedChunks[key].tiles[lx + ly * chunkSize] = t;
        loadedChunks[key].needsUpdate = true; // mark dirty
    }
}

void World::generate(unsigned int seed) {
    this->seed = seed;

    Generator generator(seed);
    generator.generate(*this);
}

bool World::updateChunks(int playerX, int playerY, int loadRadiusChunks) {
    glm::ivec2 playerChunk = getPlayerChunk(playerX, playerY);

    // Early exit if player is in the same chunk as last frame
    static glm::ivec2 lastPlayerChunk = {-9999, -9999};
    if (playerChunk == lastPlayerChunk) return false;
    lastPlayerChunk = playerChunk;

    std::unordered_map<int64_t, Chunk> newLoaded;
    bool changed = false;

    for (int dx = -loadRadiusChunks; dx <= loadRadiusChunks; ++dx) {
        for (int dy = -loadRadiusChunks; dy <= loadRadiusChunks; ++dy) {
            int cx = playerChunk.x + dx;
            int cy = playerChunk.y + dy;

            if (cx < 0 || cy < 0 || cx * chunkSize >= width ||
                cy * chunkSize >= height)
                continue;

            int64_t key = chunkKey(cx, cy);

            if (loadedChunks.count(key)) {
                newLoaded[key] = std::move(loadedChunks[key]);
            } else {
                changed = true;
                Chunk c(cx, cy, chunkSize);
                for (int tx = 0; tx < chunkSize; ++tx) {
                    for (int ty = 0; ty < chunkSize; ++ty) {
                        int worldX = cx * chunkSize + tx;
                        int worldY = cy * chunkSize + ty;
                        if (worldX >= width || worldY >= height) continue;
                        c.tiles[tx + ty * chunkSize] = tiles[worldX][worldY];
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

void World::generateChunkVertices(Chunk &chunk, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    std::vector<QuadSpec> quads;
    
    for (int tx = 0; tx < chunkSize; ++tx) {
        for (int ty = 0; ty < chunkSize; ++ty) {
            Tile &tile = chunk.tiles[tx + ty * chunkSize];
            if (!tile.isActive && tile.wallId == 0) continue;

            float wx = (chunk.chunkX * chunkSize + tx) * Constants::TileSize;
            float wy = (chunk.chunkY * chunkSize + ty) * Constants::TileSize;

            if (tile.wallId != 0) {
                const TileProperties &props = TileRegistry::wallTypes[tile.wallId];
                quads.push_back({
                    wx, wy, Constants::TileSize, Constants::TileSize, props.zValue,
                    getTexCoords(props.texCoord.x, props.texCoord.y,
                                 Constants::AtlasWidth, Constants::AtlasTileSize)
                });
            }

            if (tile.isActive) {
                const TileProperties &props = TileRegistry::tileTypes[tile.tileId];
                quads.push_back({
                    wx, wy, Constants::TileSize, Constants::TileSize, props.zValue,
                    getTexCoords(props.texCoord.x, props.texCoord.y,
                                 Constants::AtlasWidth, Constants::AtlasTileSize)
                });
            }
        }
    }

    buildMesh(vertices, indices, quads);
}

void World::save(const std::string &path, bool binary, const Inventory &inventory) const {
    fs::create_directories(fs::path(path).parent_path());

    if (binary) {
        std::ofstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("Failed to open save file: " + path);

        // Header
        f.write("TCW", 3);
        uint16_t version = 1;
        f.write(reinterpret_cast<const char*>(&version), sizeof(version));
        f.write(reinterpret_cast<const char*>(&width),   sizeof(width)  );
        f.write(reinterpret_cast<const char*>(&height),  sizeof(height) );
        f.write(reinterpret_cast<const char*>(&seed),    sizeof(seed)   );

        // Tiles - row major (x outer, y inner)
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                const Tile&t = tiles[x][y];
                f.write(reinterpret_cast<const char*>(&t.tileId),   sizeof(t.tileId)  );
                f.write(reinterpret_cast<const char*>(&t.wallId),   sizeof(t.wallId)  );
                f.write(reinterpret_cast<const char*>(&t.isActive), sizeof(t.isActive));
            }
        }

        // Inventory
        int activeSlot = inventory.activeSlot.load();
        f.write(reinterpret_cast<const char*>(&activeSlot), sizeof(activeSlot));
        for (const auto &slot : inventory.slots) {
            f.write(reinterpret_cast<const char*>(&slot.tileId), sizeof(slot.tileId));
            f.write(reinterpret_cast<const char*>(&slot.count),  sizeof(slot.count));
        }

        std::cout << "[World] Saved binary to " << path << "\n";
    } else {
        std::ofstream f(path);
        if (!f) throw std::runtime_error("Failed to open save file: " + path);

        f << "TCW 1\n";
        f << "width "  << width  << "\n";
        f << "height " << height << "\n";
        f << "seed "   << seed   << "\n";
        f << "tiles\n";

        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                const Tile &t = tiles[x][y];
                f << x << " " << y << " "
                  << t.tileId << " "
                  << t.wallId << " "
                  << t.isActive << "\n";
            }
        }

        f << "inventory\n";
        f << "activeSlot " << inventory.activeSlot.load() << "\n";
        for (int i = 0; i < INVENTORY_SLOTS; ++i) {
            f << i << " "
              << inventory.slots[i].tileId << " "
              << inventory.slots[i].count  << "\n";
        }

        std::cout << "[World] Saved text to " << path << "\n";
    }
}

bool World::load(const std::string &path, bool binary, Inventory &inventory) {
    if (!fs::exists(path)) return false;

    if (binary) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;

        // Verify magic
        char magic[3];
        f.read(magic, 3);
        if (std::strncmp(magic, "TCW", 3) != 0) {
            std::cerr << "[World] Invalid save file magic\n";
            return false;
        }

        uint16_t version;
        f.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != 1) {
            std::cerr << "[World] Unsupported save version: " << version << "\n";
            return false;
        }

        int w, h;
        f.read(reinterpret_cast<char*>(&w), sizeof(w));
        f.read(reinterpret_cast<char*>(&h), sizeof(h));
        f.read(reinterpret_cast<char*>(&seed), sizeof(seed));

        if (w != width || h != height) {
            std::cerr << "[World] Save dimensions mismatch\n";
            return false;
        }

        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                Tile &t = tiles[x][y];
                f.read(reinterpret_cast<char*>(&t.tileId), sizeof(t.tileId));
                f.read(reinterpret_cast<char*>(&t.wallId), sizeof(t.wallId));
                f.read(reinterpret_cast<char*>(&t.isActive), sizeof(t.isActive));
            }
        }

        // Inventory
        int activeSlot;
        f.read(reinterpret_cast<char*>(&activeSlot), sizeof(activeSlot));
        inventory.activeSlot.store(activeSlot);

        for (auto &slot : inventory.slots) {
            f.read(reinterpret_cast<char*>(&slot.tileId), sizeof(slot.tileId));
            f.read(reinterpret_cast<char*>(&slot.count),  sizeof(slot.count));
        }

        std::cout << "[World] Loaded binary from " << path << "\n";
    } else {
        std::ifstream f(path);
        if (!f) return false;

        std::string token;
        f >> token;
        int version;
        f >> version;

        int w, h;
        f >> token >> w;
        f >> token >> h;
        f >> token >> seed;
        f >> token; // "tiles"

        if (w != width || h != height) {
            std::cerr << "[World] Save dimensions mismatch\n";
            return false;
        }

        // Tiles
        while (f >> token) {
            if (token == "inventory") break;
            int x = std::stoi(token);
            int y;
            f >> y;
            Tile &t = tiles[x][y];
            int tileId, wallId, isActive;
            f >> tileId >> wallId >> isActive;
            t.tileId   = static_cast<uint16_t>(tileId);
            t.wallId   = static_cast<uint16_t>(wallId);
            t.isActive = static_cast<bool>(isActive);
        }

        // Inventory
        int activeSlot;
        f >> token >> activeSlot; // "activeSlot N"
        inventory.activeSlot.store(activeSlot);
        for (int i = 0; i < INVENTORY_SLOTS; ++i) {
            int idx, tileId, count;
            f >> idx >> tileId >> count;
            inventory.slots[idx].tileId = static_cast<uint32_t>(tileId);
            inventory.slots[idx].count  = count;
        }


        std::cout << "[World] Loaded text from " << path << "\n";
    }
    return true;
}
