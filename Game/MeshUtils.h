#pragma once
#include <vector>

#include "../Engine/Vertex.h"

inline void pushQuad(
    std::vector<Vertex> &vertices,
    std::vector<uint32_t> &indices,
    float x, float y, float w, float h,
    float z, const std::array<glm::vec2, 4> &texCoords,
    glm::vec3 color = {1,1,1}
) {
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{x,     y    }, z, color, texCoords[0]});
    vertices.push_back({{x + w, y    }, z, color, texCoords[1]});
    vertices.push_back({{x + w, y + h}, z, color, texCoords[2]});
    vertices.push_back({{x,     y + h}, z, color, texCoords[3]});
    indices.insert(indices.end(), {base, base+1, base+2, base+2, base+3, base});
}
