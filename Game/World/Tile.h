#pragma once
#include <cstdint>

struct Tile {
    uint16_t tileId = 0;
    uint16_t wallId = 0;
    bool isActive = false;
};