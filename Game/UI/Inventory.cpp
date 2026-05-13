#include "Inventory.h"
#include "../Rendering/MeshUtils.h"
#include "../World/Tile.h"
#include "../Constants.h"

static constexpr float SLOT_SIZE = 40.0f;
static constexpr float PADDING   = 4.0f;
static constexpr float Z_UI      = 0.05f;

void generateInventoryVertices(const Inventory &inventory, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    std::vector<QuadSpec> quads;

    float startX = PADDING;
    float startY = PADDING;

    for (int i = 0; i < INVENTORY_SLOTS; ++i) {
        float x = startX + i * (SLOT_SIZE + PADDING);
        bool isActive = (i == inventory.activeSlot);

        quads.push_back({
            x, startY, SLOT_SIZE, SLOT_SIZE, Z_UI + 0.01f,
            isActive
                ? getTexCoords(1, 255, Constants::AtlasWidth, Constants::AtlasTileSize)
                : getTexCoords(0, 255, Constants::AtlasWidth, Constants::AtlasTileSize)
        });

        // Item quad on top if slot is occupied
        if (!inventory.slots[i].empty()) {
            const TileProperties &props = TileRegistry::tileTypes[inventory.slots[i].tileId];

            quads.push_back({
                x + 4.0f, startY + 4.0f, SLOT_SIZE - 8.0f, SLOT_SIZE - 8.0f, Z_UI,
                getTexCoords(props.texCoord.x, props.texCoord.y, Constants::AtlasWidth, Constants::AtlasTileSize)
            });
        }
    }

    buildMesh(vertices, indices, quads);
}
