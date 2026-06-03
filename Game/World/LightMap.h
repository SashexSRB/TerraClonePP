#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

struct LightMap {
    // =================================================================
    // Tuning constants
    // =================================================================
    static constexpr float AMBIENT_FLOOR  = 0.045f;
    static constexpr float SKY_BRIGHTNESS = 1.0f;
    static constexpr float DECAY_AIR      = 0.84f;
    static constexpr float DECAY_WALL     = 0.60f;
    static constexpr int   MARGIN         = 6;

    // =================================================================
    // Region (set by compute, read by Game to fill push constants)
    // =================================================================
    int originX = 0;
    int originY = 0;
    int width   = 0;
    int height  = 0;

    // RGBA pixels for GPU upload (4 bytes per texel, row-major)
    std::vector<uint8_t> pixels;

    // =================================================================
    // Public interface
    // =================================================================
    template<typename WorldT>
    void compute(
        const WorldT& world,
        int camTileX,
        int camTileY,
        int visibleTilesX,
        int visibleTilesY
    );

private:
    std::vector<float> buf; // RGB floats interleaved during BFS
    inline int idx(int lx, int ly) const { return ly * width + lx; }
};