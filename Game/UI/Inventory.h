#pragma once

#include "../../Engine/Vertex.h"
#include <vector>
#include <array>
#include <cstdint>

constexpr int INVENTORY_SLOTS = 10;
constexpr int MAX_STACK_SIZE  = 999;

struct ItemStack {
    uint32_t tileId = 0;
    int      count  = 0;
    bool     empty() const { return count <= 0; }
};

struct Inventory {
    std::array<ItemStack, INVENTORY_SLOTS> slots;
    int activeSlot = 0;

    void addItem(uint32_t tileId, int count) {
        // First try to stack onto existing slot with same tile.
        for (auto &slot : slots) {
            if (!slot.empty() && slot.tileId == tileId) {
                slot.count = std::min(slot.count + count, MAX_STACK_SIZE);
                return;
            }
        }

        // Otherwise find first empty slot
        for (auto &slot : slots) {
            if (slot.empty()) {
                slot.tileId = tileId;
                slot.count = std::min(count, MAX_STACK_SIZE);
                return;
            }
        }
        // Inventory full, item lost for now.
    }

    void removeItem(int slotIndex, int count = 1) {
        if (slotIndex < 0 || slotIndex >= INVENTORY_SLOTS) return;

        slots[slotIndex].count -= count;

        if (slots[slotIndex].count <= 0) slots[slotIndex] = {};
    }

    bool hasItemInSlot(int slotIndex) const {
        return slotIndex >= 0 && slotIndex < INVENTORY_SLOTS && !slots[slotIndex].empty();
    }
};

void generateInventoryVertices(const Inventory &inventory,
                               std::vector<Vertex> &vertices,
                               std::vector<uint32_t> &indices);
