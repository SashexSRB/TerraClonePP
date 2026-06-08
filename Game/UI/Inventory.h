#pragma once

#include "Engine/Vertex.h"
#include <vector>
#include <array>
#include <atomic>

#include "Rendering/SpriteAtlas.h"

constexpr int INVENTORY_SLOTS = 10;
constexpr int MAX_STACK_SIZE  = 999;

struct ItemStack {
    uint32_t itemId = 0;
    int      count  = 0;
    bool     empty() const { return count <= 0; }
};

struct Inventory {
    std::array<ItemStack, INVENTORY_SLOTS> slots;

    /**
     * Active inventory slot used by gameplay (thread-safe index).
     */
    std::atomic<int> activeSlot = 0;

    /**
     * Adds items to inventory.
     *
     * Behavior:
     * - First attempts to stack into existing item stacks
     * - If none exist, fills first empty slot
     * - Excess items beyond capacity are discarded
     *
     * @param itemId Type identifier of item being added
     * @param count Number of items to add
     */
    void addItem(uint32_t itemId, int count) {
        // First try to stack onto existing slot with same tile.
        for (auto &slot : slots) {
            if (!slot.empty() && slot.itemId == itemId) {
                slot.count = std::min(slot.count + count, MAX_STACK_SIZE);
                return;
            }
        }

        // Otherwise find first empty slot
        for (auto &slot : slots) {
            if (slot.empty()) {
                slot.itemId = itemId;
                slot.count = std::min(count, MAX_STACK_SIZE);
                return;
            }
        }
        // Inventory full, item lost for now.
    }

    /**
     * Removes items from a specific inventory slot.
     *
     * If count exceeds available items, slot is cleared.
     *
     * @param slotIndex Slot index in range [0, INVENTORY_SLOTS)
     * @param count Number of items to remove
     */
    void removeItem(int slotIndex, int count = 1) {
        if (slotIndex < 0 || slotIndex >= INVENTORY_SLOTS) return;

        slots[slotIndex].count -= count;

        if (slots[slotIndex].count <= 0) slots[slotIndex] = {};
    }

    /**
     * Checks whether a slot contains an item.
     *
     * @param slotIndex Slot index in range [0, INVENTORY_SLOTS)
     * @return True if slot contains at least one item
     */
    bool hasItemInSlot(int slotIndex) const {
        return slotIndex >= 0 && slotIndex < INVENTORY_SLOTS && !slots[slotIndex].empty();
    }
};

/**
 * Generates GPU-ready vertex/index buffers for inventory rendering.
 *
 * @param inventory Source inventory data
 * @param vertices Output vertex buffer (cleared/filled)
 * @param indices Output index buffer (cleared/filled)
 */
void generateInventoryVertices(
    const Inventory& inventory,
    std::vector<Vertex>& tileVerts,
    std::vector<uint32_t>& tileIndices,
    std::vector<Vertex>& spriteVerts,
    std::vector<uint32_t>& spriteIndices,
    const SpriteAtlas &atlas
);
