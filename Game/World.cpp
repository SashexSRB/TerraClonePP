#include "World.h"
#include "MeshUtils.h"
#include "Constants.h"
#include "../Lib/PerlinNoise.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
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

    float frequency = 0.05f; // smoothness of terrain
    float amplitude = 10.0f; // height variation
    int groundLevel = height / 2;

    // Generate terrain height for each column
    std::vector<int> surfaceHeight(width);

    for (int x = 0; x < width; ++x) {
        // Simple noise using sine + rng offset
        double noise = perlin.octave2D_01(x * frequency, 0.0, 4) * 2.0 - 1.0;

        int terrainHeight = groundLevel + static_cast<int>(noise * amplitude);
        surfaceHeight[x] = terrainHeight;
    }

    // Assign tiles based on terrain height
    for (int x = 0; x < width; ++x) {
        int terrainHeight = surfaceHeight[x];

        for (int y = 0; y < height; ++y) {
            Tile &tile = tiles[x][y];

            if (y < terrainHeight) {
                tile.tileId = 0;
                tile.isActive = false;
            } else if (y == terrainHeight) {
                tile.tileId = 3;
                tile.isActive = true;
            } else if (y < terrainHeight + 5) {
                tile.tileId = 1;
                tile.isActive = true;
            } else {
                tile.tileId = 2;
                tile.isActive = true;
            }
            tile.wallId = (y > terrainHeight) ? 1 : 0;
        }

        // carve caves underground
        /*
        for (int y = terrainHeight + 20; y < height; ++y) { // start deeper
          double caveNoise = perlin.octave2D_01(
              x * 0.08, y * 0.03, 3); // adjust frequency and octaves if needed

          if (caveNoise > 0.65) { // thresold controls cave density
            Tile &tile = tiles[x][y];
            tile.tileId = 0;
            tile.isActive = false;
          }
        }
        */
    }
}

bool World::updateChunks(int playerX, int playerY, int loadRadiusChunks) {
    glm::ivec2 playerChunk = getPlayerChunk(playerX, playerY);
    std::unordered_map<int64_t, Chunk> newLoaded;
    bool changed = false;

    for (int dx = -loadRadiusChunks; dx <= loadRadiusChunks; ++dx) {
        for (int dy = -loadRadiusChunks; dy <= loadRadiusChunks; ++dy) {
            int cx = playerChunk.x + dx;
            int cy = playerChunk.y + dy;

            // Clamp to world bounds
            if (cx < 0 || cy < 0 || cx * chunkSize >= width ||
                cy * chunkSize >= height)
                continue;

            int64_t key = chunkKey(cx, cy);

            // keep existing chunk if loaded
            if (loadedChunks.count(key)) {
                newLoaded[key] = std::move(loadedChunks[key]);
            } else {
                changed = true;
                // Otherwise, generate new chunk from tiles
                Chunk c(cx, cy, chunkSize);
                for (int tx = 0; tx < chunkSize; ++tx) {
                    for (int ty = 0; ty < chunkSize; ++ty) {
                        int worldX = cx * chunkSize + tx;
                        int worldY = cy * chunkSize + ty;
                        if (worldX >= width || worldY >= height)
                            continue;
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
    vertices.clear();
    indices.clear();
    
    for (int tx = 0; tx < chunkSize; ++tx) {
        for (int ty = 0; ty < chunkSize; ++ty) {
            Tile &tile = chunk.tiles[tx + ty * chunkSize];

            if (!tile.isActive && tile.wallId == 0) continue;

            int worldX = chunk.chunkX * chunkSize + tx;
            int worldY = chunk.chunkY * chunkSize + ty;

            // Background wall
            if (tile.wallId != 0) {
                const TileProperties &props = TileRegistry::wallTypes[tile.wallId];
                auto texCoords = getTexCoords(
                    static_cast<int>(props.texCoord.x),
                    static_cast<int>(props.texCoord.y), Constants::AtlasWidth, Constants::AtlasTileSize
                );

                pushQuad(
                    vertices, indices,
                    worldX * Constants::TileSize, worldY * Constants::TileSize,
                    Constants::TileSize, Constants::TileSize, props.zValue, texCoords
                );
            }

            // Foreground tile
            if (tile.isActive) {
                const TileProperties &props = TileRegistry::tileTypes[tile.tileId];
                auto texCoords = getTexCoords(
                    static_cast<int>(props.texCoord.x),
                    static_cast<int>(props.texCoord.y), Constants::AtlasWidth, Constants::AtlasTileSize
                );

                pushQuad(
                    vertices, indices,
                    worldX * Constants::TileSize, worldY * Constants::TileSize,
                    Constants::TileSize, Constants::TileSize, props.zValue, texCoords
                );
            }
        }
    }
}
