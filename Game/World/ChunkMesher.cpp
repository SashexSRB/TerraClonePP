#include "ChunkMesher.h"
#include "Chunk.h"
#include "Constants.h"

#include "Tile.h"
#include "Items/Item.h"
#include "Rendering/MeshUtils.h"

namespace ChunkMesher {
    void mesh(const Chunk& chunk, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int chunkSize, std::vector<QuadSpec>& quads) {
        quads.clear();

        for (int tx = 0; tx < chunkSize; ++tx) {
            for (int ty = 0; ty < chunkSize; ++ty) {
                const Tile& tile = chunk.tiles[tx + ty * chunkSize];

                if (!tile.isActive && tile.wallId == 0) continue;

                const float wx = static_cast<float>(chunk.chunkX * chunkSize + tx) * Constants::TileSize;
                const float wy = static_cast<float>(chunk.chunkY * chunkSize + ty) * Constants::TileSize;

                if (tile.wallId != 0) {
                    const GameItem& item = Registry::get(tile.wallId);
                    quads.push_back({
                        wx, wy, Constants::TileSize, Constants::TileSize, item.zValue,
                        getTexCoords(item.texX, item.texY, Constants::AtlasWidth, Constants::AtlasTileSize)
                    });
                }

                if (tile.isActive) {
                    const GameItem& item = Registry::get(tile.tileId);
                    quads.push_back({
                        wx, wy, Constants::TileSize, Constants::TileSize, item.zValue,
                        getTexCoords(item.texX, item.texY, Constants::AtlasWidth, Constants::AtlasTileSize)
                    });
                }
            }
        }
        buildMesh(vertices, indices, quads);
    }
}
