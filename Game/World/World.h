#pragma once

#include "Tile.h"
#include "Chunk.h"
#include "../../Engine/Vertex.h"

#include <string>
#include <unordered_map>
#include <vector>




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
