#pragma once

#include "World.h"
#include <glm/glm.hpp>
#include <vector>

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

struct Player {
    glm::vec2 position = {0.0f, 0.0f};
    glm::vec2 velocity = {0.0f, 0.0f};
    float moveSpeed = 200.0f;
    float jumpSpeed = 400.0f;
    bool isGrounded = false;
    Inventory inventory;
};

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices,
                            std::vector<uint32_t> &indices);

void generateInventoryVertices(const Inventory &inventory, float screenWidth,
                               float screenHeight,
                               std::vector<Vertex> &vertices,
                               std::vector<uint32_t> &indices);
