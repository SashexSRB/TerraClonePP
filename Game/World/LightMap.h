#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

struct LightMap {
    struct RGB { float r, g, b; };

    struct Cell {
        int16_t lx, ly;
        float   r, g, b;
    };

    // =================================================================
    // Tuning constants
    // =================================================================
    static constexpr float AMBIENT_FLOOR  = 0.005f;
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

    /**
     * RGBA lightmap output buffer.
     * Stored in row-major order (4 bytes per pixel).
     * Used directly for GPU upload.
     */
    std::vector<uint8_t> pixels;

    // =================================================================
    // Public interface
    // =================================================================

    /**
     * Computes light propagation for a visible world region.
     *
     * @param world World data source used for light occlusion and emission
     * @param camTileX Camera center X position in tile space
     * @param camTileY Camera center Y position in tile space
     * @param visibleTilesX Horizontal lightmap coverage in tiles
     * @param visibleTilesY Vertical lightmap coverage in tiles
     *
     * @note Output is written into `pixels` and region metadata
     *       (originX/originY/width/height).
     */
    template<typename WorldT>
    void compute(
        const WorldT& world,
        int camTileX,
        int camTileY,
        int visibleTilesX,
        int visibleTilesY
    );

private:
    std::vector<RGB>     buf;
    std::vector<uint8_t> solid;
    std::vector<Cell>    bfsQueue;
};