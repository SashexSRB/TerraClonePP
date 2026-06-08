#include "Player.h"
#include "Rendering/MeshUtils.h"
#include "Constants.h"

void generatePlayerVertices(const Player &player, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices, const SpriteAtlas& atlas) {
    float px = player.position.x;
    float py = player.position.y;
    float pw = Constants::PlayerWidth;
    float ph = Constants::PlayerHeight;

    if (atlas.has(SPRITE_PLAYER)) {
        // Single quad covering the full player hitbox, using sprite UV
        SpriteRect uv = atlas.get(SPRITE_PLAYER);

        buildMesh(vertices, indices, {{
            px, py, pw, ph, Constants::PlayerZ,
            getSpriteCoords(uv.u0, uv.v0, uv.u1, uv.v1)
        }});

    } else {
        // Fallback: old atlas tile grid layout
        float tw = Constants::TileSize;
        buildMesh(vertices, indices, {
            { px,      py,          tw, tw, Constants::PlayerZ,
              getTexCoords(254, 253, Constants::AtlasWidth, Constants::AtlasTileSize) },
            { px + tw, py,          tw, tw, Constants::PlayerZ,
              getTexCoords(255, 253, Constants::AtlasWidth, Constants::AtlasTileSize) },
            { px,      py + tw,     tw, tw, Constants::PlayerZ,
              getTexCoords(254, 254, Constants::AtlasWidth, Constants::AtlasTileSize) },
            { px + tw, py + tw,     tw, tw, Constants::PlayerZ,
              getTexCoords(255, 254, Constants::AtlasWidth, Constants::AtlasTileSize) },
            { px,      py + tw * 2, tw, tw, Constants::PlayerZ,
              getTexCoords(254, 255, Constants::AtlasWidth, Constants::AtlasTileSize) },
            { px + tw, py + tw * 2, tw, tw, Constants::PlayerZ,
              getTexCoords(255, 255, Constants::AtlasWidth, Constants::AtlasTileSize) },
        });
    }
}


