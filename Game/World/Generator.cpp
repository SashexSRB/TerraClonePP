#include "Generator.h"
#include "World.h"

#include <algorithm>

Generator::Generator(unsigned int seed)
    : seed(seed),
      terrainNoise(seed),
      biomeNoise(seed + 1),
      caveNoise(seed + 2)
{}

static const std::array<BiomeData, 3> biomeTable = {{
    {3,1,0.0030f,0.012f,60.f,15.f}, // forest
    {4,5,0.0025f,0.010f,35.f, 8.f}, // desert
    {6,7,0.0035f,0.014f,90.f,25.f}  // snow
}};

void Generator::generate(World& world) {
    biomeMap.resize(world.getWidth());
    surfaceHeight.resize(world.getWidth());

    generateBiomeMap(world);
    generateHeightMap(world);

    paintTerrain(world);

    generateCaves(world);

    // optional:
    // smoothCaves(world, 2);
}

void Generator::generateBiomeMap(World& world) {
    const float biomeFreq = 0.0007f;

    for (int x = 0; x < world.getWidth(); ++x) {
        float desertNoise = biomeNoise.octave2D_01(x * biomeFreq, 1000.0, 3);
        float forestNoise = biomeNoise.octave2D_01(x * biomeFreq, 2000.0, 3);
        float snowNoise   = biomeNoise.octave2D_01(x * biomeFreq, 3000.0, 3);

        float total = desertNoise + forestNoise + snowNoise;

        biomeMap[x] = {
            forestNoise / total,
            desertNoise / total,
            snowNoise / total
        };
    }
}

void Generator::generateHeightMap(World& world) {
    const int groundLevel = world.getHeight() / 2;

    auto sampleTerrain = [&](int x, const BiomeData& data) {
        double base   = terrainNoise.octave2D_01(x * data.baseFreq,  0.0,  4) * 2.0 - 1.0;
        double detail = terrainNoise.octave2D_01(x * data.detailFreq,100.0,2) * 2.0 - 1.0;

        return base * data.baseAmplitude + detail * data.detailAmplitude;
    };

    for (int x = 0; x < world.getWidth(); ++x) {
        const auto& weights = biomeMap[x];

        float forest = sampleTerrain(x, getBiomeData(Biome::Forest));
        float desert = sampleTerrain(x, getBiomeData(Biome::Desert));
        float snow   = sampleTerrain(x, getBiomeData(Biome::Snow));

        int finalHeight = groundLevel + static_cast<int>(
            forest * weights.forest +
            desert * weights.desert +
            snow * weights.snow
        );

        surfaceHeight[x] = std::clamp(
            finalHeight,
            10,
            world.getHeight() - 10
        );
    }
}

Biome Generator::getDominantBiome(int x) const {
    const auto& w = biomeMap[x];
    if (w.forest > w.desert && w.forest > w.snow) return Biome::Forest;
    if (w.desert > w.snow) return Biome::Desert;
    return Biome::Snow;
}

void Generator::paintTerrain(World& world) {
    for (int x = 0; x < world.getWidth(); ++x) {
        Biome dominant = getDominantBiome(x);

        const auto& data = getBiomeData(dominant);
        int terrainHeight = surfaceHeight[x];

        int dirtDepth = 20 + static_cast<int>(
            terrainNoise.noise2D_01(x * 0.05, 500.0) * 25
        );

        const auto& weights = biomeMap[x];

        bool transition = std::max({
            weights.forest,
            weights.desert,
            weights.snow
        }) < 0.75f;

        for (int y = 0; y < world.getHeight(); ++y) {
            Tile& tile = world.getTile(x, y);

            if (y < terrainHeight) {
                tile.tileId = 0;
                tile.isActive = false;
            } else if (y == terrainHeight) {
                tile.tileId =data.surfaceTile;
                if (transition) {
                    double scatter = biomeNoise.noise2D_01(
                        x * 0.08,
                        y * 0.08
                    );

                    if (scatter > 0.65) {
                        if (weights.forest > 0.3f) tile.tileId = 3;
                        else if (weights.desert > 0.3f) tile.tileId = 4;
                        else tile.tileId = 6;
                    }
                }

                tile.isActive = true;
            } else if (y < terrainHeight + dirtDepth) {
                tile.tileId = data.subsurfaceTile;
                tile.isActive = true;
            } else {
                tile.tileId = 2;
                tile.isActive = true;
            }

            tile.wallId = (y >= terrainHeight) ? 100 : 0;
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

int Generator::countSolidNeighbors(World& world, int x, int y) const {
    int count = 0;

    for (int ox = -1; ox <= 1; ++ox) {
        for (int oy = -1; oy <= 1; ++oy) {

            if (ox == 0 && oy == 0) continue;

            int nx = x + ox;
            int ny = y + oy;

            if (nx < 0 || ny < 0 || nx >= world.getWidth() || ny >= world.getHeight()) {
                count++;
                continue;
            }

            if (world.getTile(nx, ny).isActive) count++;
        }
    }

    return count;
}

void Generator::smoothCaves(World& world, int iterations) {
    const int width =world.getWidth();
    const int height =world.getHeight();

    std::vector<uint8_t> solid(width * height);

    for (int i = 0; i < iterations; ++i) {
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {

                int neighbors = countSolidNeighbors(
                    world,
                    x,
                    y
                );

                bool current = world.getTile(x, y).isActive;

                bool next;

                if (neighbors > 5)
                    next = true;

                else if (neighbors < 3)
                    next = false;

                else
                    next = current;

                solid[x + y * width] = next;
            }
        }

        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                bool active = solid[x + y * width];

                Tile& tile = world.getTile(x, y);

                tile.isActive = active;

                if (!active) tile.tileId = 0;
            }
        }
    }
}

bool Generator::isSolid(World& world, int x, int y) const {
    if (x < 0 || y < 0 || x >= world.getWidth() || y >= world.getHeight()) return true;

    return world.getTile(x, y).isActive;
}

const BiomeData& Generator::getBiomeData(Biome biome) const {
    return biomeTable[static_cast<int>(biome)];
}