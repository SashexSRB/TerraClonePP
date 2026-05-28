#pragma once

#include "Chunk.h"
#include "ChunkManager.h"
#include "Generator.h"
#include "Engine/Vertex.h"
#include "UI/Inventory.h"

#include <string>
#include <unordered_map>
#include <vector>

class World {
public:
    World(int w, int h);

    int getWidth()  const { return width; }
    int getHeight() const { return height; }
    unsigned int getSeed() const { return seed; }
    void setSeed(unsigned int s) { seed = s; }

    Tile       &getTile(int x, int y);
    const Tile &getTile(int x, int y) const;
    void        setTile(int x, int y, Tile t);

    void generate(unsigned int seed);

    ChunkManager chunks;
private:
    std::vector<std::vector<Tile>> tiles;
    int width, height;
    unsigned int seed = 0;
};
