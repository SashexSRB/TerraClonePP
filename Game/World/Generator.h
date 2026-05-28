#pragma once

#include "Lib/PerlinNoise.hpp"

class World;

enum class Biome : uint8_t {
    Forest,
    Desert,
    Snow
};

struct BiomeData {
    uint16_t surfaceTile;
    uint16_t subsurfaceTile;
    float baseFreq;
    float detailFreq;
    float baseAmplitude;
    float detailAmplitude;
};

struct BiomeSegment {
    Biome biome;
    int startX;
    int endX;
    int priority;
};

class Generator {
public:
    explicit Generator(unsigned int seed);

    void generate(World& world);

private:
    unsigned int seed;

    siv::PerlinNoise terrainNoise;
    siv::PerlinNoise biomeNoise;
    siv::PerlinNoise caveNoise;

    std::vector<BiomeSegment> segments;
    std::vector<int> surfaceHeight;

    void initSegments(int worldWidth);
    Biome getBiomeAt(int x) const;
    const BiomeData& getBiomeData(Biome biome) const;


    void generateHeightMap(World& world);
    void paintTerrain(World& world);
    void generateCaves(World& world);

    // void generateOres(World& world);
    // void generateDecorations(World& world);

    bool isSolid(World& world, int x, int y) const;
};