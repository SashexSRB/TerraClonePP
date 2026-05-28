#include "Generator.h"
#include "World.h"

#include <algorithm>

Generator::Generator(unsigned int seed)
    : seed(seed),
      terrainNoise(seed),
      biomeNoise(seed + 1),
      caveNoise(seed + 2)
{}

static const BiomeData biomeTable[] = {
    {3,1,0.0030f,0.012f,60.f,15.f}, // forest
    {4,5,0.0025f,0.010f,35.f, 8.f}, // desert
    {6,7,0.0035f,0.014f,90.f,25.f}  // snow
};

const BiomeData& Generator::getBiomeData(Biome biome) const {
    return biomeTable[static_cast<int>(biome)];
}

void Generator::initSegments(int w) {
    int center = w / 2;
    segments.clear();

    // Left side
    segments.push_back({Biome::Forest, 0,   w/8,    3});
    segments.push_back({Biome::Snow,   w/8, w/3,    4});
    segments.push_back({Biome::Forest, w/3, center, 2});

    // Right side
    segments.push_back({Biome::Forest, center, w*2/3, 2});
    segments.push_back({Biome::Desert, w*2/3,  w*7/8, 4});
    segments.push_back({Biome::Forest, w*7/8,  w,     3});
}

Biome Generator::getBiomeAt(int x) const {
    Biome best = Biome::Forest;
    int bestPriority = -1;

    for (const auto& s : segments) {
        if (x < s.startX || x > s.endX) continue;

        if (s.priority > bestPriority) {
            bestPriority = s.priority;
            best = s.biome;
        }
    }

    return best;
}

void Generator::generate(World& world) {
    int w = world.getWidth();
    int h = world.getHeight();

    surfaceHeight.resize(world.getWidth());
    initSegments(w);

    generateHeightMap(world);
    paintTerrain(world);
    generateCaves(world);
}

void Generator::generateHeightMap(World& world) {
    int w = world.getWidth();
    int h = world.getHeight();

    const int groundLevel = h / 2;

    for (int x = 0; x < w; ++x) {
        Biome b = getBiomeAt(x);
        const BiomeData& d = getBiomeData(b);

        double base = terrainNoise.octave2D_01(x * d.baseFreq, 0.0, 4) * 2.0 - 1.0;
        double detail = terrainNoise.octave2D_01(x * d.detailFreq, 100.0, 2) * 2.0 - 1.0;

        const int height = groundLevel + static_cast<int>(base * d.baseAmplitude + detail * d.detailAmplitude);

        surfaceHeight[x] = std::clamp(height, 10, h - 10);
    }
}

void Generator::paintTerrain(World& world) {
    int w = world.getWidth();
    int h = world.getHeight();

    for (int x = 0; x < w; ++x) {
        Biome b = getBiomeAt(x);
        const BiomeData& d = getBiomeData(b);

        int terrainHeight = surfaceHeight[x];

        for (int y = 0; y < h; ++y) {
            Tile& t = world.getTile(x, y);

            if (y < terrainHeight) {
                t.tileId = 0;
                t.isActive = false;
            } else if (y == terrainHeight) {
                t.tileId = d.surfaceTile;
                t.isActive = true;
            } else if (y < terrainHeight + 20) {
                t.tileId = d.subsurfaceTile;
                t.isActive = true;
            } else {
                t.tileId = 2;
                t.isActive = true;
            }

            t.wallId = (y >= terrainHeight) ? 100 : 0;
        }
    }
}

void Generator::generateCaves(World& world) {
    const float caveFreq = 0.045f;
    const int caveMinDepth = 20;

    for (int x = 0; x < world.getWidth(); ++x) {
        int startY = surfaceHeight[x] + caveMinDepth;

        for (int y = startY; y < world.getHeight(); ++y) {
            double caveMask = caveNoise.octave2D_01(
                x * 0.003,
                y * 0.003,
                2
            );

            if (caveMask < 0.42) continue;

            double warpX = caveNoise.noise2D_01(
                x * 0.01,
                y * 0.01
            ) * 25.0;

            double warpY = caveNoise.noise2D_01(
                (x + 999) * 0.01,
                (y + 999) * 0.01
            ) * 25.0;

            double noise =caveNoise.octave2D_01(
                (x + warpX) * caveFreq,
                (y + warpY) * caveFreq,
                3
            );

            double largeNoise = caveNoise.octave2D_01(
                x * 0.015,
                y * 0.015,
                1
            );

            float depth = static_cast<float>(y) / world.getHeight();

            float threshold = std::lerp(
                0.72f,
                0.58f,
                depth
            );

            bool carve = noise > threshold || largeNoise > 0.78;

            if (carve) {
                Tile& tile = world.getTile(x, y);

                tile.tileId = 0;
                tile.isActive = false;
            }
        }
    }
}

bool Generator::isSolid(World& world, int x, int y) const {
    if (x < 0 || y < 0 || x >= world.getWidth() || y >= world.getHeight()) return true;

    return world.getTile(x, y).isActive;
}