#include "Inventory.h"
#include "../Rendering/MeshUtils.h"
#include "../World/Tile.h"
#include "../Constants.h"

void generateInventoryVertices(const Inventory &inventory,
                               std::vector<Vertex> &vertices,
                               std::vector<uint32_t> &indices) {
    vertices.clear();
    indices.clear();

    const float slotSize = 40.0f; // UI slot size
    const float padding = 4.0f;
    float startX = padding;
    float startY = padding;


    for (size_t i = 0; i < inventory.items.size() && i < 10; ++i) {
        float x = startX + i * (slotSize + padding);

        uint32_t tileId = inventory.items[i].first;
        const TileProperties &props = TileRegistry::tileTypes[tileId];

        auto texCoords = getTexCoords(
            props.texCoord.x, props.texCoord.y,
            Constants::AtlasWidth, Constants::AtlasTileSize
        );

        pushQuad(
            vertices, indices,
            x, startY,
            slotSize, slotSize,
            0.05f, texCoords
        );
    }
}