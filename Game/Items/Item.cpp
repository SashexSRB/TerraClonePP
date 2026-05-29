#include "Item.h"
#include <stdexcept>
#include <iostream>

std::unordered_map<uint32_t, GameItem> Registry::items;

void Registry::initialize() {
// ── Tiles ────────────────────────────────────────────────────────────
    items[0] = {0, "Air",       ItemType::Tile, ToolType::None, 0, 0, false, 0.0f, TileBreakType::None,    0,    0.0f};
    items[1] = {1, "Dirt",      ItemType::Tile, ToolType::None, 1, 0, true,  0.2f, TileBreakType::Pickaxe, 0,    0.5f};
    items[2] = {2, "Stone",     ItemType::Tile, ToolType::None, 2, 0, true,  0.2f, TileBreakType::Pickaxe, 50,   1.5f};
    items[3] = {3, "Grass",     ItemType::Tile, ToolType::None, 3, 0, true,  0.2f, TileBreakType::Pickaxe, 0,    0.5f};
    items[4] = {4, "Sand",      ItemType::Tile, ToolType::None, 4, 0, true,  0.2f, TileBreakType::Pickaxe, 0,    0.4f};
    items[5] = {5, "Sandstone", ItemType::Tile, ToolType::None, 5, 0, true,  0.2f, TileBreakType::Pickaxe, 35,   1.0f};
    items[6] = {6, "Snow",      ItemType::Tile, ToolType::None, 6, 0, true,  0.2f, TileBreakType::Pickaxe, 0,    0.3f};
    items[7] = {7, "Ice",       ItemType::Tile, ToolType::None, 7, 0, true,  0.2f, TileBreakType::Pickaxe, 35,   0.8f};

// ── Walls ────────────────────────────────────────────────────────────
    items[100] = {100, "StoneWall", ItemType::Wall, ToolType::None, 0, 1, false, 0.9f, TileBreakType::Hammer, 50, 0.5f};

// ── Tools ────────────────────────────────────────────────────────────
    items[1000] = {1000, "Copper Pickaxe", ItemType::Tool,   ToolType::Pickaxe, 0, 2, false, 0.0f, TileBreakType::None, 0, 0.0f, 55, 1.0f, 0,  1};
    items[1001] = {1001, "Copper Axe",     ItemType::Tool,   ToolType::Axe,     1, 2, false, 0.0f, TileBreakType::None, 0, 0.0f, 35, 1.0f, 0,  1};
    items[1002] = {1002, "Copper Sword",   ItemType::Weapon, ToolType::None,    2, 2, false, 0.0f, TileBreakType::None, 0, 0.0f, 0,  1.0f, 15, 1};

    std::cout << "OK: Registry initialized with " << items.size() << " items.\n";
}

const GameItem& Registry::get(uint32_t id) {
    auto it = items.find(id);
    if (it == items.end()) {
        throw std::runtime_error("Registry: unknown item id: "  + std::to_string(id));
    }
    return it->second;
}

bool Registry::isValid(uint32_t id) {
    return items.count(id) > 0;
}
