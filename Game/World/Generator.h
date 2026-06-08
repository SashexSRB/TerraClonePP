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


/**
 * Procedural world generator responsible for terrain creation using
 * layered Perlin noise (terrain, biome, caves).
 */
class Generator {
public:

    /**
     * @param seed Deterministic seed used for all noise generation
     */
    explicit Generator(unsigned int seed);

    /**
     * Generates a full world (terrain, biomes, caves).
     */
    void generate(World& world);

private:
    unsigned int seed;

    siv::PerlinNoise terrainNoise;
    siv::PerlinNoise biomeNoise;
    siv::PerlinNoise caveNoise;

    std::vector<BiomeSegment> segments;
    std::vector<int> surfaceHeight;

    void initSegments(int worldWidth);

    /**
     * Returns biome type at a given x-position using biome noise.
     */
    Biome getBiomeAt(int x) const;

    /**
     * Retrieves generation parameters for a biome type.
     */
    const BiomeData& getBiomeData(Biome biome) const;


    void generateHeightMap(World& world);
    void paintTerrain(World& world);
    void generateCaves(World& world);

    // void generateOres(World& world);
    // void generateDecorations(World& world);

    /**
     * Checks whether a world tile is solid terrain.
     */
    bool isSolid(World& world, int x, int y) const;
};