#include "Player.h"
#include "../Rendering/MeshUtils.h"
#include "../Constants.h"

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {

    float px = player.position.x;
    float py = player.position.y;
    float tw = Constants::TileSize;

    buildMesh(vertices, indices, {
        // Top row
        { px,      py,          tw, tw, 0.1f, getTexCoords(254, 253, Constants::AtlasWidth, Constants::AtlasTileSize) },
        { px + tw, py,          tw, tw, 0.1f, getTexCoords(255, 253, Constants::AtlasWidth, Constants::AtlasTileSize) },
        // Middle row
        { px,      py + tw,     tw, tw, 0.1f, getTexCoords(254, 254, Constants::AtlasWidth, Constants::AtlasTileSize) },
        { px + tw, py + tw,     tw, tw, 0.1f, getTexCoords(255, 254, Constants::AtlasWidth, Constants::AtlasTileSize) },
        // Bottom row
        { px,      py + tw * 2, tw, tw, 0.1f, getTexCoords(254, 255, Constants::AtlasWidth, Constants::AtlasTileSize) },
        { px + tw, py + tw * 2, tw, tw, 0.1f, getTexCoords(255, 255, Constants::AtlasWidth, Constants::AtlasTileSize) },
    });
}


