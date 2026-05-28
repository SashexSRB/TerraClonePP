#pragma once

#include <array>
#include "Vertex.h"
#include <vector>
#include <cstdint>
#include "Rendering/MeshUtils.h"

struct Chunk;

namespace ChunkMesher {
    void mesh(const Chunk& chunk, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int ChunkSize, std::vector<QuadSpec>& scratch);
}