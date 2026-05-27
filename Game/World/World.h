#pragma once

#include "Chunk.h"
#include "Generator.h"
#include "../../Engine/Vertex.h"
#include "../UI/Inventory.h"

#include <string>
#include <unordered_map>
#include <vector>

class World {
private:
    std::vector<std::vector<Tile>> tiles;
    int width, height;

public:
    World(int w, int h);
    int chunkSize = 64;
    std::unordered_map<int64_t, Chunk> loadedChunks;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    unsigned int getSeed() const { return seed; }

    Tile &getTile(int x, int y);
    void setTile(int x, int y, Tile t);

    int64_t chunkKey(int x, int y);
    std::string chunkMeshKey(int cx, int cy) {
        return "chunk_" + std::to_string(cx) + "_" + std::to_string(cy);
    };
    glm::ivec2 getPlayerChunk(int playerX, int playerY) const;

    bool updateChunks(int playerX, int playerY, int loadRadiusChunks);

    void generate(unsigned int seed);
    void generateChunkVertices(Chunk &chunk, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);

    void save(const std::string &path, bool binary, const Inventory &inventory) const;
    bool load(const std::string &path, bool binary, Inventory &inventory);
private:
    unsigned int seed = 0;
};
