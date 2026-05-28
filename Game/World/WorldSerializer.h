#pragma once

#include <string>

class World;
struct Inventory;

class WorldSerializer {
public:
    WorldSerializer(const std::string& path);

    void save(const World& world, const Inventory& inventory) const;
    bool load(World& world, Inventory& inventory) const;

private:
    std::string path;
};