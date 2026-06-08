#pragma once

#include <array>
#include "Vertex.h"
#include <vector>
#include <cstdint>
#include "Rendering/MeshUtils.h"

struct Chunk;

namespace ChunkMesher {

    /**
     * Generates mesh data for a chunk.
     *
     * @param chunk Input world chunk used as voxel/tile source
     * @param vertices Output vertex buffer (cleared/filled by function)
     * @param indices Output index buffer (cleared/filled by function)
     * @param ChunkSize Size of the chunk in tiles (defines mesh bounds)
     * @param scratch Temporary buffer used for intermediate quad generation
     */
    void mesh(const Chunk& chunk, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int ChunkSize, std::vector<QuadSpec>& scratch);
}