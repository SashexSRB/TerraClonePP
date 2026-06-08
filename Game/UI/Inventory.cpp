#include "Inventory.h"
#include "Rendering/MeshUtils.h"
#include "Items/Item.h"
#include "Constants.h"

static constexpr float SLOT_SIZE = 40.0f;
static constexpr float PADDING   = 4.0f;
static constexpr float Z_UI      = 0.05f;

void generateInventoryVertices(const Inventory& inventory,
                               std::vector<Vertex>& tileVerts,
                               std::vector<uint32_t>& tileIndices,
                               std::vector<Vertex>& spriteVerts,
                               std::vector<uint32_t>& spriteIndices,
                               const SpriteAtlas& atlas) {
    std::vector<QuadSpec> tileQuads;
    std::vector<QuadSpec> spriteQuads;

    float startX = PADDING;
    float startY = PADDING;

    for (int i = 0; i < INVENTORY_SLOTS; ++i) {
        float x        = startX + i * (SLOT_SIZE + PADDING);
        bool  isActive = (i == inventory.activeSlot);

        tileQuads.push_back({
            x, startY, SLOT_SIZE, SLOT_SIZE, Z_UI + 0.01f,
            isActive
                ? getTexCoords(1, 255, Constants::AtlasWidth, Constants::AtlasTileSize)
                : getTexCoords(0, 255, Constants::AtlasWidth, Constants::AtlasTileSize)
        });

        if (!inventory.slots[i].empty()) {
            uint32_t        itemId = inventory.slots[i].itemId;
            const GameItem& item   = Registry::get(itemId);

            float ix = x + 4.0f;
            float iy = startY + 4.0f;
            float iw = SLOT_SIZE - 8.0f;
            float ih = SLOT_SIZE - 8.0f;

            if (!item.spritePath.empty() && atlas.has(itemId)) {
                SpriteRect uv = atlas.get(itemId);
                spriteQuads.push_back({
                    ix, iy, iw, ih, Z_UI,
                    getSpriteCoords(uv.u0, uv.v0, uv.u1, uv.v1)
                });
            } else {
                tileQuads.push_back({
                    ix, iy, iw, ih, Z_UI,
                    getTexCoords(item.texX, item.texY, Constants::AtlasWidth, Constants::AtlasTileSize)
                });
            }
        }
    }

    buildMesh(tileVerts,   tileIndices,   tileQuads);
    buildMesh(spriteVerts, spriteIndices, spriteQuads);
}