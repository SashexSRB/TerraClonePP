#include "Inventory.h"
#include "../Rendering/MeshUtils.h"
#include "../World/Tile.h"
#include "../Constants.h"

static constexpr float SLOT_SIZE = 40.0f;
static constexpr float PADDING   = 4.0f;
static constexpr float Z_UI      = 0.05f;

void generateInventoryVertices(const Inventory &inventory, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) {
    vertices.clear();
    indices.clear();

    float startX = PADDING;
    float startY = PADDING;

    for (int i = 0; i < INVENTORY_SLOTS; ++i) {
        float x = startX + i * (SLOT_SIZE + PADDING);

        // Slot background - dark grey for normal, lighter for active
        bool isActive = (i == inventory.activeSlot);

        // Use highlight or normal slot texture
        auto bgTexCoords = isActive
            ? getTexCoords(6, 0, Constants::AtlasWidth, Constants::AtlasTileSize)
            : getTexCoords(5, 0, Constants::AtlasWidth, Constants::AtlasTileSize);

        pushQuad(vertices, indices, x, startY, SLOT_SIZE, SLOT_SIZE, Z_UI + 0.01f, bgTexCoords);

        // Item quad on top if slot is occupied
        if (!inventory.slots[i].empty()) {
            const TileProperties &props = TileRegistry::tileTypes[inventory.slots[i].tileId];
            auto texCoords = getTexCoords(props.texCoord.x, props.texCoord.y, Constants::AtlasWidth, Constants::AtlasTileSize);
            pushQuad(vertices, indices, x + 4.0f, startY + 4.f, SLOT_SIZE - 8.0f, SLOT_SIZE - 8.0f, Z_UI, texCoords);
        }
    }
}
