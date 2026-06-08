#pragma once

#include <string>

class World;
struct Inventory;

/**
 * Handles serialization and deserialization of game state
 * (world data and player inventory) to a persistent file.
 */
class WorldSerializer {
public:

    /**
     * @param path Filesystem path used for saving/loading world data.
     */
    WorldSerializer(const std::string& path);

    /**
     * Saves world state and inventory to disk.
     *
     * @param world Current world state to serialize
     * @param inventory Current player inventory state to serialize
     */
    void save(const World& world, const Inventory& inventory) const;

    /**
     * Loads world state and inventory from disk.
     *
     * @param world Output world instance to populate
     * @param inventory Output inventory instance to populate
     *
     * @return True if loading succeeded, false if file is missing or invalid
     */
    bool load(World& world, Inventory& inventory) const;

private:
    std::string path;
};