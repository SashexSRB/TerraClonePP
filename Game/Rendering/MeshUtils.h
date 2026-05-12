#pragma once
#include <vector>
#include "../../Engine/Vertex.h"

void pushQuad(
    std::vector<Vertex> &vertices,
    std::vector<uint32_t> &indices,
    float x, float y, float w, float h,
    float z, const std::array<glm::vec2, 4> &texCoords,
    glm::vec3 color = {1,1,1}
);