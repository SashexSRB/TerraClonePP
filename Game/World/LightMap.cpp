#include "LightMap.h"
#include "World.h"
#include <queue>

template<typename WorldT>
void LightMap::compute(const WorldT& world,
                       int camTileX, int camTileY,
                       int visibleTilesX, int visibleTilesY)
{
    // ---- 1. Region ----------------------------------------------------------
    originX = camTileX - visibleTilesX / 2 - MARGIN;
    originY = camTileY - visibleTilesY / 2 - MARGIN;
    width   = visibleTilesX + MARGIN * 2;
    height  = visibleTilesY + MARGIN * 2;

    if (originX < 0) { width  += originX; originX = 0; }
    if (originY < 0) { height += originY; originY = 0; }
    width  = std::min(width,  world.getWidth()  - originX);
    height = std::min(height, world.getHeight() - originY);

    const int N = width * height;

    buf.resize(N);      std::fill(buf.begin(),   buf.end(),   RGB{0,0,0});
    solid.resize(N);    std::fill(solid.begin(), solid.end(), 0);
    bfsQueue.resize(N);
    pixels.resize(N * 4);

    int qHead = 0, qTail = 0;

    // ---- 2. Solid mask + sky seeds ------------------------------------------
    for (int lx = 0; lx < width; ++lx) {
        int wx = originX + lx;
        if (wx >= world.getWidth()) continue;

        bool skyOpen = true;
        for (int sy = 0; sy < originY && skyOpen; ++sy)
            if (world.getTile(wx, sy).isActive) skyOpen = false;

        for (int ly = 0; ly < height; ++ly) {
            int wy = originY + ly;
            if (wy >= world.getHeight()) break;

            int i        = ly * width + lx;
            bool isSolid = world.getTile(wx, wy).isActive;
            solid[i]     = isSolid ? 1 : 0;

            if (skyOpen && !isSolid) {
                buf[i] = {SKY_BRIGHTNESS, SKY_BRIGHTNESS, SKY_BRIGHTNESS};
                bfsQueue[qTail++ % N] = {
                    (int16_t)lx, (int16_t)ly,
                    SKY_BRIGHTNESS, SKY_BRIGHTNESS, SKY_BRIGHTNESS
                };
            } else if (skyOpen && isSolid) {
                skyOpen = false;
            }
        }
    }

    // ---- 3. BFS -------------------------------------------------------------
    const int dx[] = { 1, -1,  0,  0 };
    const int dy[] = { 0,  0,  1, -1 };

    while (qHead != qTail) {
        auto [clx, cly, cr, cg, cb] = bfsQueue[qHead++ % N];

        // Prune dim cells early
        if (std::max({cr, cg, cb}) < 0.005f) continue;

        for (int d = 0; d < 4; ++d) {
            int nlx = clx + dx[d];
            int nly = cly + dy[d];
            if (nlx < 0 || nlx >= width || nly < 0 || nly >= height) continue;

            int   ni    = nly * width + nlx;
            float decay = solid[ni] ? DECAY_WALL : DECAY_AIR;

            float nr = cr * decay;
            float ng = cg * decay;
            float nb = cb * decay;

            RGB& cur     = buf[ni];
            bool improved = false;

            if (nr > cur.r + 0.001f) { cur.r = nr; improved = true; }
            if (ng > cur.g + 0.001f) { cur.g = ng; improved = true; }
            if (nb > cur.b + 0.001f) { cur.b = nb; improved = true; }

            if (improved)
                bfsQueue[qTail++ % N] = {
                    (int16_t)nlx, (int16_t)nly,
                    cur.r, cur.g, cur.b
                };
        }
    }

    // ---- 4. Ambient floor + pack --------------------------------------------
    for (int i = 0; i < N; ++i) {
        pixels[i*4+0] = static_cast<uint8_t>(
            std::min(std::max(buf[i].r, AMBIENT_FLOOR), 1.0f) * 255.0f);
        pixels[i*4+1] = static_cast<uint8_t>(
            std::min(std::max(buf[i].g, AMBIENT_FLOOR), 1.0f) * 255.0f);
        pixels[i*4+2] = static_cast<uint8_t>(
            std::min(std::max(buf[i].b, AMBIENT_FLOOR), 1.0f) * 255.0f);
        pixels[i*4+3] = 255;
    }
}

// Explicit instantiation so the linker finds it from Game.cpp
template void LightMap::compute<World>(const World&, int, int, int, int);


























