#include "World.h"
#include "../Constants.h"
#include "../Rendering/MeshUtils.h"
#include "../../Lib/PerlinNoise.hpp"

#include <unordered_map>

std::unordered_map<uint16_t, TileProperties> TileRegistry::tileTypes;
std::unordered_map<uint16_t, TileProperties> TileRegistry::wallTypes;
int World::chunkSize = 64;
std::unordered_map<int64_t, Chunk> World::loadedChunks;

int64_t World::chunkKey(int x, int y) {
    return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(y);
}

glm::ivec2 getPlayerChunk(int playerX, int playerY) {
    return {playerX / World::chunkSize, playerY / World::chunkSize};
}

void TileRegistry::initialize() {
    static const std::vector<TileDefinition> tileDefs = {
        {0, "Air", 0, 0, false, 0.0f},
        {1, "Dirt", 1, 0, true, 0.2f},
        {2, "Stone", 2, 0, true, 0.2f},
        {3, "Grass", 3, 0, true, 0.2f},
        {4, "Sand", 7,0, true, 0.2f},
        {5, "Sandstone", 8,0, true, 0.2f},
        {6, "Snow", 9,0, true, 0.2f},
        {7, "Ice", 10,0, true, 0.2f},
    };

    static const std::vector<TileDefinition> wallDefs = {
        {0, "None", 0, 1, false, 1.0f},
        {1, "StoneWall", 1, 1, false, 1.0f},
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
    siv::PerlinNoise perlin(seed);
    siv::PerlinNoise biomeNoise(seed + 1); // separate noise for biome variation

    // ─── Biome layout ───────────────────────────────────────────────────────
    // Randomly assign snow/desert to left/right based on seed
    bool snowOnLeft = (seed % 2 == 0);

    // Biome regions in tile units
    int centerStart  = width / 2 - 600;  // forest start
    int centerEnd    = width / 2 + 600;  // forest end
    int transitionW  = 200;              // blend zone width in tiles

    // ─── Terrain ────────────────────────────────────────────────────────────
    const float baseFreq      = 0.003f;
    const float detailFreq    = 0.015f;
    const float baseAmplitude = 60.0f;
    const float detailAmp     = 15.0f;
    const int   groundLevel   = height / 2;

    std::vector<int> surfaceHeight(width);
    for (int x = 0; x < width; ++x) {
        double base   = perlin.octave2D_01(x * baseFreq,   0.0, 4) * 2.0 - 1.0;
        double detail = perlin.octave2D_01(x * detailFreq, 1.0, 2) * 2.0 - 1.0;
        int h = groundLevel
              + static_cast<int>(base   * baseAmplitude)
              + static_cast<int>(detail * detailAmp);
        surfaceHeight[x] = std::clamp(h, 10, height - 10);
    }

    // ─── Biome blend helper ─────────────────────────────────────────────────
    // Returns 0.0 = full left biome, 1.0 = full forest, blends in transition zones
    auto getBiomeBlend = [&](int x) -> float {
        if (x < centerStart - transitionW) return 0.0f;           // full side biome
        if (x < centerStart)                                        // left transition
            return static_cast<float>(x - (centerStart - transitionW)) / transitionW;
        if (x <= centerEnd) return 1.0f;                           // full forest
        if (x < centerEnd + transitionW)                           // right transition
            return 1.0f - static_cast<float>(x - centerEnd) / transitionW;
        return 0.0f;                                               // full side biome
    };

    auto getBiome = [&](int x) -> int {
        // 0 = forest, 1 = snow, 2 = desert
        if (x >= centerStart && x <= centerEnd) return 0;
        bool isLeft = (x < centerStart);
        bool isSnow = (isLeft == snowOnLeft);
        return isSnow ? 1 : 2;
    };

    // ─── Tile assignment ────────────────────────────────────────────────────
    for (int x = 0; x < width; ++x) {
        int   terrainHeight = surfaceHeight[x];
        int   biome         = getBiome(x);
        float blend         = getBiomeBlend(x);

        for (int y = 0; y < height; ++y) {
            Tile &tile = tiles[x][y];

            if (y < terrainHeight) {
                tile.tileId  = 0;
                tile.isActive = false;
            } else if (y == terrainHeight) {
                // Surface tile depends on biome
                if (biome == 1)       tile.tileId = 6; // snow
                else if (biome == 2)  tile.tileId = 4; // sand
                else                  tile.tileId = 3; // grass

                // Transition zones blend toward forest
                if (blend > 0.0f && blend < 1.0f) {
                    // Use a noise value to scatter biome tiles in transition
                    double scatter = biomeNoise.noise2D_01(x * 0.1, y * 0.1);
                    if (scatter < blend) tile.tileId = 3; // grass wins
                }
                tile.isActive = true;
            } else if (y < terrainHeight + 6) {
                // Subsurface layer
                if (biome == 1)      tile.tileId = 7; // ice
                else if (biome == 2) tile.tileId = 5; // sandstone
                else                 tile.tileId = 1; // dirt

                if (blend > 0.0f && blend < 1.0f) {
                    double scatter = biomeNoise.noise2D_01(x * 0.1, y * 0.1);
                    if (scatter < blend) tile.tileId = 1; // dirt wins
                }
                tile.isActive = true;
            } else {
                tile.tileId  = 2; // stone everywhere deep
                tile.isActive = true;
            }

            tile.wallId = (y >= terrainHeight) ? 1 : 0;
        }
    }

    // ─── Caves ──────────────────────────────────────────────────────────────
    const float caveFreqX    = 0.04f;
    const float caveFreqY    = 0.04f;
    const float caveThresh   = 0.62f;
    const int   caveMinDepth = 16;

    for (int x = 0; x < width; ++x) {
        int caveStart = surfaceHeight[x] + caveMinDepth;
        for (int y = caveStart; y < height; ++y) {
            double noise = perlin.octave2D_01(x * caveFreqX, y * caveFreqY, 3);
            if (noise > caveThresh) {
                tiles[x][y].tileId   = 0;
                tiles[x][y].isActive = false;
            }
        }
    }
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
