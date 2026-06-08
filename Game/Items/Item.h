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

/**
 * Defines a single game item entry in the global item database.
 *
 * This struct acts as a data-driven definition for all items,
 * including tiles, tools, weapons, and consumables.
 *
 * NOTE: Many fields are only valid depending on `itemType`.
 */
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

/**
 * Global registry of all game items.
 *
 * Provides centralized access to item definitions by ID.
 */
class Registry {
public:
    /**
     * Initializes the item database.
     * Must be called before any `get()` usage.
     */
    static void initialize();

    /**
     * Retrieves an item definition by ID.
     *
     * @param id Item identifier
     * @return Reference to immutable item definition
     *
     * @note Behavior is undefined if ID is invalid unless checked first
     */
    static const GameItem& get(uint32_t id);

    /**
     * Checks whether an item ID exists in the registry.
     */
    static bool isValid(uint32_t id);

    /**
     * Global item storage database.
     */
    static std::unordered_map<uint32_t, GameItem> items;
};