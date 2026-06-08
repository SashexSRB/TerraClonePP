#pragma once

#include "glm/vec2.hpp"
#include <vector>

struct QuadSpec {
    float x, y;
    float w, h;
    float z;
    std::array<glm::vec2, 4> texCoords;
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
};

/**
 * Computes normalized texture coordinates from a texture atlas.
 *
 * @param x Tile X index in atlas
 * @param y Tile Y index in atlas
 * @param atlasSize Size of the full texture atlas in pixels
 * @param tileSize Size of a single tile in pixels
 *
 * @return Array of UV coordinates in clockwise order starting from top-left
 */
inline std::array<glm::vec2, 4> getTexCoords(int x, int y, int atlasSize = 256, int tileSize = 8) {
    float step = static_cast<float>(tileSize) / static_cast<float>(atlasSize);

    float u0 = x * step;
    float v0 = y * step;
    float u1 = u0 + step;
    float v1 = v0 + step;

    return {
            {
                {u0, v0}, // top-left
                {u1, v0}, // top-right
                {u1, v1}, // bottom-right
                {u0, v1}, // bottom-left
            }
    };
}

/**
 * Appends a quad to vertex and index buffers.
 *
 * @param vertices Output vertex buffer (appended to)
 * @param indices Output index buffer (appended to)
 * @param q Quad definition in world space
 */
inline void pushQuad(
    std::vector<Vertex> &vertices,
    std::vector<uint32_t> &indices,
    const QuadSpec &q
) {
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{q.x,          q.y          }, q.z, q.color, q.texCoords[0]});
    vertices.push_back({{q.x + q.w, q.y          }, q.z, q.color, q.texCoords[1]});
    vertices.push_back({{q.x + q.w, q.y + q.h }, q.z, q.color, q.texCoords[2]});
    vertices.push_back({{q.x,       q.y + q.h    }, q.z, q.color, q.texCoords[3]});
    indices.insert(indices.end(), {base, base+1, base+2, base+2, base+3, base});
}


/**
 * Builds a full mesh from a set of quads.
 *
 * @param vertices Output vertex buffer (cleared and rebuilt)
 * @param indices Output index buffer (cleared and rebuilt)
 * @param quads Input quad list used to generate geometry
 */
inline void buildMesh(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, const std::vector<QuadSpec> &quads) {
    vertices.clear();
    indices.clear();
    for (const auto &q : quads) {
        pushQuad(vertices, indices, q);
    }
}