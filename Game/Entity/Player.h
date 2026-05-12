#pragma once

#include "../UI/Inventory.h"
#include <glm/glm.hpp>

struct Player {
    glm::vec2 position = {0.0f, 0.0f};
    glm::vec2 velocity = {0.0f, 0.0f};
    float moveSpeed = 200.0f;
    float jumpSpeed = 400.0f;
    bool isGrounded = false;
    Inventory inventory;
};

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);