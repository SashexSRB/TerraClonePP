#include "Player.h"
#include "../Rendering/MeshUtils.h"
#include "../Constants.h"

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    vertices.clear();
    indices.clear();

    auto texCoords = getTexCoords(4, 0, Constants::AtlasWidth, Constants::AtlasTileSize);

    pushQuad(
        vertices, indices,
        player.position.x, player.position.y,
        Constants::TileSize, Constants::TileSize,
        0.1f, texCoords
    );
}


