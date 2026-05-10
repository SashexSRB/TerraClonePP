#include "Player.h"

#include "Constants.h"
#include "MeshUtils.h"

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices,
                            std::vector<uint32_t> &indices) {
    vertices.clear();
    indices.clear();


    const float texTileSize = static_cast<float>(Constants::AtlasTileSize) / static_cast<float>(Constants::AtlasWidth);
    const std::array<glm::vec2, 4> texCoords = {
        {
            {0.0f,        texTileSize},
            {texTileSize, texTileSize},
            {texTileSize, 0.0f       },
            {0.0f,        0.0f       }
        }
    };

    pushQuad(
        vertices, indices,
        player.position.x, player.position.y,
        Constants::TileSize, Constants::TileSize,
        0.1f, texCoords
    );
}


