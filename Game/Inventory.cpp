#include "Inventory.h"

#include "Constants.h"
#include "MeshUtils.h"

void generateInventoryVertices(const Inventory &inventory, float screenWidth,
                               float screenHeight,
                               std::vector<Vertex> &vertices,
                               std::vector<uint32_t> &indices) {
    vertices.clear();
    indices.clear();
    const float slotSize = 40.0f; // UI slot size
    float y = screenHeight - slotSize; // Bottom of screen

    for (size_t i = 0; i < inventory.items.size() && i < 10; ++i) {
        float x = i * slotSize;
        uint32_t tileId = inventory.items[i].first;
        const TileProperties &props = TileRegistry::tileTypes[tileId];
        auto texCoords = getTexCoords(props.texCoord.x, props.texCoord.y, Constants::AtlasWidth, Constants::AtlasTileSize);

        pushQuad(
            vertices, indices,
            x, y,
            slotSize, slotSize,
            0.9f, texCoords
        );
    }
}