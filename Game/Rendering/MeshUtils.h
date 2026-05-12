#pragma once

#include "../../Engine/Vertex.h"
#include <vector>

struct QuadSpec {
    float x, y;
    float w, h;
    float z;
    std::array<glm::vec2, 4> texCoords;
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
};

inline void pushQuad(
    std::vector<Vertex> &vertices,
    std::vector<uint32_t> &indices,
    const QuadSpec &q
) {
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{q.x,       q.y      }, q.z, q.color, q.texCoords[0]});
    vertices.push_back({{q.x + q.w, q.y      }, q.z, q.color, q.texCoords[1]});
    vertices.push_back({{q.x + q.w, q.y + q.h}, q.z, q.color, q.texCoords[2]});
    vertices.push_back({{q.x,       q.y + q.h}, q.z, q.color, q.texCoords[3]});
    indices.insert(indices.end(), {base, base+1, base+2, base+2, base+3, base});
}

inline void buildMesh(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, const std::vector<QuadSpec> &quads) {
    vertices.clear();
    indices.clear();
    for (const auto &q : quads) {
        pushQuad(vertices, indices, q);
    }
}