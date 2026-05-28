#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

enum class ItemType : uint8_t {
    None,
    Tile,
    Wall,
    Tool,
    Weapon,
    Consumable,
    Accessory
};

enum class ToolType : uint8_t {
    None,
    Pickaxe,
    Axe,
    Hammer
};

enum class TileBreakType : uint8_t {
    None,
    Pickaxe,
    Axe,
    Hammer
};

struct GameItem {
    uint32_t      id            = 0;
    std::string   name          = "UndefinedItem";
    ItemType      itemType      = ItemType::None;
    ToolType      toolType      = ToolType::None;

    // Atlas coords for world rendering and inventory icon
    int           texX          = 0;
    int           texY          = 0;

    // Item properties (only if itemType == Tile or Wall)
    bool          isSolid       = false;
    float         zValue        = 0;
    TileBreakType breakType     = TileBreakType::None;
    int           requiredPower = 0;
    float         breakTime     = 0.0f;

    // Tool properties (only if itemType == Tool)
    int           toolPower     = 0;
    float         toolSpeed     = 1.0f;

    // Weapon properties (only if itemType == Weapon)
    int           damage        = 0;

    int           maxStack      = 999;
};

class Registry {
public:
    static std::unordered_map<uint32_t, GameItem> items;
    static void initialize();
    static const GameItem& get(uint32_t id);
    static bool isValid(uint32_t id);
};