#include "Player.h"
#include "../Rendering/MeshUtils.h"
#include "../Constants.h"

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    buildMesh(vertices, indices, {
        { // Top tile
            player.position.x, player.position.y,
            Constants::PlayerWidth, Constants::TileSize,
            0.1f,
            getTexCoords(254, 255, Constants::AtlasWidth, Constants::AtlasTileSize)
        },
        { // Bottom tile
            player.position.x, player.position.y  + Constants::TileSize,
            Constants::PlayerWidth, Constants::TileSize,
            0.1f,
            getTexCoords(255, 255, Constants::AtlasWidth, Constants::AtlasTileSize)
        }
    });
}


