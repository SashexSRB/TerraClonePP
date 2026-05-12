#pragma once

#include "../Engine/Vertex.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct Tile {
    uint16_t tileId = 0;
    uint16_t wallId = 0;
    bool isActive = false;
};

struct TileProperties {
    std::string name;
    glm::ivec2 texCoord;
    bool isSolid = true;
    float zValue = 0.5f;
};

struct TileDefinition {
    uint16_t id;
    std::string name;
    int texX, texY;
    bool isSolid;
    float zValue;
};

struct Chunk {
    int chunkX, chunkY;
    std::vector<Tile> tiles;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool needsUpdate = true;

    Chunk() = default;

    Chunk(int x, int y, int chunkSize) : chunkX(x), chunkY(y) {
        tiles.resize(chunkSize * chunkSize);
    }
};

class TileRegistry {
public:
    static std::unordered_map<uint16_t, TileProperties> tileTypes;
    static std::unordered_map<uint16_t, TileProperties> wallTypes;

    static void initialize();
};

class World {
private:
    std::vector<std::vector<Tile> > tiles;
    int width, height;

public:
    World(int w, int h);
    static int chunkSize;
    static std::unordered_map<int64_t, Chunk> loadedChunks;

    Tile &getTile(int x, int y);
    void setTile(int x, int y, Tile t);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    static int64_t chunkKey(int x, int y);
    static std::string chunkMeshKey(int cx, int cy) {
        return "chunk_" + std::to_string(cx) + "_" + std::to_string(cy);
    };

    bool updateChunks(int playerX, int playerY, int loadRadiusChunks);
    void generate(unsigned int seed);
    void generateChunkVertices(Chunk &chunk, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);
};
