#include "World.h"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>

std::unordered_map<uint16_t, TileProperties> TileRegistry::tileTypes;
std::unordered_map<uint16_t, TileProperties> TileRegistry::wallTypes;

void TileRegistry::initialize() {
  static const std::vector<TileDefinition> tileDefs = {
      {0, "Air", 0, 0, false, 0.0f},
      {1, "Dirt", 1, 0, true, 0.5f},
      {2, "Stone", 2, 0, true, 0.5f},
      {3, "Grass", 3, 0, true, 0.5f},
  };

  static const std::vector<TileDefinition> wallDefs = {
      {0, "None", 0, 1, false, 1.0f},
      {1, "StoneWall", 1, 1, false, 1.0f},
  };

  for (const auto &def : tileDefs) {
    tileTypes[def.id] = {
        def.name, {def.texX, def.texY}, def.isSolid, def.zValue};
  }

  for (const auto &def : wallDefs) {
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

Tile &World::getTile(int x, int y) { return tiles[x][y]; }

void World::generate(unsigned int seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  float frequency = 0.05f; // smoothness of terrain
  float amplitude = 10.0f; // height variation
  int groundLevel = height / 2;

  // Generate terrain height for each column
  std::vector<int> surfaceHeight(width);
  for (int x = 0; x < width; ++x) {
    // Simple noise using sine + rng offset
    float noise = std::sin(x * frequency) + dist(rng) * 0.5f;
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
  }
}

void World::generateVertices(std::vector<Vertex> &vertices,
                             std::vector<uint32_t> &indices) {
  vertices.clear();
  indices.clear();
  const float tileSize = 32.0f;       // Pixels per tile in world
  const size_t maxVertices = 1000000; // Arbitrary limit for safety (uint32_t
                                      // supports much more (4 billion))
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; y++) {
      const Tile &tile = tiles[x][y];

      if (!tile.isActive && tile.wallId == 0) // Skip empty tiles
        continue;

      if (vertices.size() >= maxVertices - 4)
        throw std::runtime_error("Too many vertices for buffer");

      // Generate wall vertices (background)
      if (tile.wallId != 0) {
        const TileProperties &props = TileRegistry::wallTypes[tile.wallId];
        auto texCoords =
            getTexCoords(static_cast<int>(props.texCoord.x),
                         static_cast<int>(props.texCoord.y), 256, 8);
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

        vertices.push_back({{x * tileSize, y * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[0]});
        vertices.push_back({{(x + 1) * tileSize, y * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[1]});
        vertices.push_back({{(x + 1) * tileSize, (y + 1) * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[2]});
        vertices.push_back({{x * tileSize, (y + 1) * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[3]});

        indices.insert(indices.end(),
                       {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2,
                        baseIndex + 3, baseIndex});
      }

      // Generate tile vertices (foreground)
      if (tile.isActive) {
        const TileProperties &props = TileRegistry::tileTypes[tile.tileId];
        auto texCoords =
            getTexCoords(static_cast<int>(props.texCoord.x),
                         static_cast<int>(props.texCoord.y), 256, 8);
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

        vertices.push_back({{x * tileSize, y * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[0]});
        vertices.push_back({{(x + 1) * tileSize, y * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[1]});
        vertices.push_back({{(x + 1) * tileSize, (y + 1) * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[2]});
        vertices.push_back({{x * tileSize, (y + 1) * tileSize},
                            props.zValue,
                            {1.0f, 1.0f, 1.0f},
                            texCoords[3]});

        indices.insert(indices.end(),
                       {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2,
                        baseIndex + 3, baseIndex});
      }
    }
  }
  std::cout << "World generated: " << vertices.size() << " vertices, "
            << indices.size() << " indices.\n";
}
