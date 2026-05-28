#pragma once
#include <cstdint>

struct Tile {
    uint32_t tileId = 0;
    uint32_t wallId = 0;
    bool isActive = false;
};