#pragma once

#include <vector>
#include "World.h"
#include "../Engine/Vertex.h"

struct Inventory {
    std::vector<std::pair<uint16_t, int> > items;

    void addItem(uint16_t tileId, int count) {
        for (auto &item: items) {
            if (item.first == tileId) {
                item.second += count;
                return;
            }
        }
        items.push_back({tileId, count});
    }
};

void generateInventoryVertices(const Inventory &inventory, float screenWidth,
                               float screenHeight,
                               std::vector<Vertex> &vertices,
                               std::vector<uint32_t> &indices);
