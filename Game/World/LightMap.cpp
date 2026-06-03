#include "LightMap.h"
#include "World.h"
#include <queue>

template<typename WorldT>
void LightMap::compute(const WorldT& world, int camTileX, int camTileY, int visibleTilesX, int visibleTilesY) {
    // --- 1. Region ---
    originX = camTileX - visibleTilesX / 2 - MARGIN;
    originY = camTileY - visibleTilesY / 2 - MARGIN;
    width   = visibleTilesX + MARGIN * 2;
    height  = visibleTilesY + MARGIN * 2;

    // Clamp to world bounds
    if (originX < 0) { width  += originX; originX = 0; }
    if (originY < 0) { height += originY; originY = 0; }
    width  = std::min(width,  world.getWidth()  - originX);
    height = std::min(height, world.getHeight() - originY);

    const int N = width * height;
    buf.assign(N * 3, 0.0f);
    pixels.resize(N * 4);

    std::vector<bool> solid(N, false);

    struct Cell { int i; float r, g, b; };
    std::queue<Cell> q;

    // --- 2. Solid mask + sky seed sources ---
    for (int lx = 0; lx < width; ++lx) {
        int wx = originX + lx;
        if (wx >= world.getWidth()) continue;

        // Determine if sky is open above our region in this column
        bool skyOpen = true;
        for (int sy = 0; sy < originY && skyOpen; ++sy) {
            if (world.getTile(wx, sy).isActive) skyOpen = false;
        }

        for (int ly = 0; ly < height; ++ly) {
            int wy = originY + ly;
            if (wy >= world.getHeight()) continue;

            int i = idx(lx, ly);
            bool isSolid = world.getTile(wx, wy).isActive;
            solid[i] = isSolid;

            if (skyOpen && !isSolid) {
                buf[i * 3 + 0] = SKY_BRIGHTNESS;
                buf[i * 3 + 1] = SKY_BRIGHTNESS;
                buf[i * 3 + 2] = SKY_BRIGHTNESS;
                q.push({i, SKY_BRIGHTNESS, SKY_BRIGHTNESS, SKY_BRIGHTNESS});
            } else if (skyOpen && isSolid) {
                skyOpen = false;
            }
        }
    }

    // --- 3. BFS flood fill
    const int dx[] = { 1, -1, 0, 0 };
    const int dy[] = { 0, 0, 1, -1 };

    while (!q.empty()) {
        auto [ci, cr, cg, cb] = q.front();
        q.pop();

        int clx = ci % width;
        int cly = ci / width;

        for (int d = 0; d < 4; ++d) {
            int nlx = clx + dx[d];
            int nly = cly + dy[d];
            if (nlx < 0 || nlx >= width || nly < 0 || nly >= height) continue;

            int ni = idx(nlx, nly);
            float decay = solid[ni] ? DECAY_WALL : DECAY_AIR;

            float nr = cr * decay;
            float ng = cg * decay;
            float nb = cb * decay;

            bool improved = false;
            if (nr > buf[ni * 3 + 0] + 0.001f) { buf[ni * 3 + 0] = nr; improved = true; }
            if (ng > buf[ni * 3 + 1] + 0.001f) { buf[ni * 3 + 1] = ng; improved = true; }
            if (nb > buf[ni * 3 + 2] + 0.001f) { buf[ni * 3 + 2] = nb; improved = true; }

            if (improved) q.push({ ni, buf[ni * 3 + 0], buf[ni * 3 + 1], buf[ni * 3 + 2] } );
        }
    }

    // --- 4. Ambient floor + pack to RGBA uint8 ---
    for (int i = 0; i < N; ++i) {
        float r = std::min(std::max(buf[i * 3 + 0], AMBIENT_FLOOR), 1.0f);
        float g = std::min(std::max(buf[i * 3 + 1], AMBIENT_FLOOR), 1.0f);
        float b = std::min(std::max(buf[i * 3 + 2], AMBIENT_FLOOR), 1.0f);

        pixels[i * 4 + 0] = static_cast<uint8_t>(r * 255.0f);
        pixels[i * 4 + 1] = static_cast<uint8_t>(g * 255.0f);
        pixels[i * 4 + 2] = static_cast<uint8_t>(b * 255.0f);
        pixels[i * 4 + 3] = 255;
    }
}

// Explicit instantiation so the linker finds it from Game.cpp
template void LightMap::compute<World>(const World&, int, int, int, int);


























