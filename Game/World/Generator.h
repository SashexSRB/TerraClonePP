#pragma once

#include "../Lib/PerlinNoise.hpp"

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

struct BiomeWeights {
    float forest;
    float desert;
    float snow;
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

    std::vector<BiomeWeights> biomeMap;
    std::vector<int> surfaceHeight;

    void generateBiomeMap(World& world);
    void generateHeightMap(World& world);

    void paintTerrain(World& world);

    void generateCaves(World& world);
    void smoothCaves(World& world, int iterations);

    void generateOres(World& world);
    void generateDecorations(World& world);

    bool isSolid(World& world, int x, int y) const;
    int countSolidNeighbors(World& world, int x, int y) const;

    Biome getDominantBiome(int x) const;

    const BiomeData& getBiomeData(Biome biome) const;;
};