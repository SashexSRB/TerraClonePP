#pragma once

#include <vector>
#include <cstdint>

struct Vertex;
struct Chunk;

namespace ChunkMesher {
    void mesh(const Chunk& chunk, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int ChunkSize);
}