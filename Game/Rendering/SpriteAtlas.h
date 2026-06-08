#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <glm/glm.hpp>

constexpr uint32_t SPRITE_PLAYER = 0xFFFF0001u;

struct SpriteRect {
    // Normalized UV coords in the atlas texture
    float u0, v0, u1, v1;
};

struct SpriteEntry {
    uint32_t    id;
    std::string path; // full path to PNG file
    int         width;
    int         height;
};

/**
 * Stitches individual PNG files into one RGBA texture at init, returns normalized UV rects for each registered sprite.
 *
 * A sprite is identified by a uint32_t id that matches the GameItem id,
 * or a dedicated constant for the player
 */
class SpriteAtlas {
public:
    /**
     * Register a sprite before calling build().
     *
     * @param id Matches the GameItem id, or uses SPRITE_PLAYER.
     */
    void add(uint32_t id, const std::string& path);

    /**
     * Stitch all registered sprites into a single RGBA atlas.
     *
     * Called once during init.
     *
     * @param atlasH Atlas height
     * @param atlasW Atlas width
     */
    std::vector<uint8_t> build(int& atlasW, int& atlasH);

    /**
     * @param id
     * @return UV rect for a registered sprite.
     * @return Zeroed rect if ID is not found.
     */
    SpriteRect get(uint32_t id) const;

    bool has(uint32_t id) const;

    int atlasWidth = 0;
    int atlasHeight = 0;

private:
    std::vector<SpriteEntry> entries;
    std::unordered_map<uint32_t, SpriteRect> uvMap;
};