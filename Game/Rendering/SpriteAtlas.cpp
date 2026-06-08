#include "SpriteAtlas.h"

#include <algorithm>

#include "Lib/stb_image.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cmath>

void SpriteAtlas::add(uint32_t id, const std::string& path) {
    entries.push_back({id,path, 0,0});
}

std::vector<uint8_t> SpriteAtlas::build(int& outW, int& outH) {
    if (entries.empty()) {
        outW = outH = 1;
        atlasWidth = atlasHeight = 1;
        uvMap.clear();
        return {255, 255, 255, 255};
    }

    // Load all images
    struct LoadedSprite {
        uint32_t id;
        int      w, h;
        std::vector<uint8_t> pixels;
    };

    std::vector<LoadedSprite> loaded;
    loaded.reserve(entries.size());

    for (auto& e : entries) {
        int w, h, ch;
        stbi_uc* raw = stbi_load(e.path.c_str(), &w, &h, &ch, STBI_rgb_alpha);

        if (!raw) {
            // Missing file - generate a solid magenta placeholder so it's visually obvious without crashing.
            std::cout << "[SpriteAtlas] Missing sprite: " << e.path << " - using placeholder!\n";

            w = h = 32;
            std::vector<uint8_t> placeholder(w * h * 4);
            for (int i = 0; i < w * h; ++i) {
                placeholder[i * 4 + 0] = 255;
                placeholder[i * 4 + 1] = 0;
                placeholder[i * 4 + 2] = 255;
                placeholder[i * 4 + 3] = 255;
            }
            loaded.push_back({e.id, w, h, placeholder});
        } else {
            std::vector<uint8_t> pixels(raw, raw + w * h * 4);
            stbi_image_free(raw);
            loaded.push_back({e.id, w, h, std::move(pixels)});
        }
    }

    // Simple row-pack layout
    // Sort by height descending for better packing
    std::sort(loaded.begin(), loaded.end(), [](const LoadedSprite& a, const LoadedSprite& b) {
       return a.h > b.h;
    });

    // Find the atlas width: next power of two >= widest sprite * 4 or sum,
    // capped at 4096. Just do a single horizontal row per sprite for simplicity, sprites are usually few and generally small.
    int totalW = 0;
    int maxH   = 0;
    for (auto& s : loaded) {
        totalW += s.w;
        maxH = std::max(maxH, s.h);
    }

    // ROund atlas width up to next power of two for GPU compatibility
    int aw = 1;
    while (aw < totalW) aw <<= 1;
    int ah = 1;
    while (ah < maxH) ah <<= 1;

    atlasWidth = aw;
    atlasHeight = ah;
    outW = aw;
    outH = ah;

    std::vector<uint8_t> atlas(aw * ah * 4, 0);

    // Blit each sprite and record UV
    int penX = 0;
    for (auto& s : loaded) {
        for (int row = 0; row < s.h; ++row) {
            const uint8_t* src = s.pixels.data() + row * s.w * 4;
            uint8_t* dst = atlas.data() + (row * aw + penX) * 4;
            memcpy(dst, src, s.w * 4);
        }

        SpriteRect uv;
        uv.u0 = static_cast<float>(penX) / aw;
        uv.v0 = 0.0f;
        uv.u1 = static_cast<float>(penX + s.w) / aw;
        uv.v1 = static_cast<float>(s.h) / ah;
        uvMap[s.id] = uv;

        penX += s.w;
    }

    std::cout << "[SpriteAtlas] Built << " << loaded.size() << " sprites into " << aw << "x" << ah << " atlas.\n";
    return atlas;
}

SpriteRect SpriteAtlas::get(uint32_t id) const {
    auto it = uvMap.find(id);
    if (it == uvMap.end()) return {0, 0, 0, 0};
    return it->second;
}

bool SpriteAtlas::has(uint32_t id) const {
    return uvMap.count(id) > 0;
}