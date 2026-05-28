#include "WorldSerializer.h"
#include "World.h"
#include "UI/Inventory.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

WorldSerializer::WorldSerializer(const std::string& path) : path(path) {}

void WorldSerializer::save(const World& world, const Inventory& inventory) const {
    fs::create_directories(fs::path(path).parent_path());

    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open file for writing" + path);

    auto write = [&](const auto& value) {
        f.write(reinterpret_cast<const char *>(&value), sizeof(value));
    };

    f.write("TCW", 3);
    uint16_t version = 1;
    int w = world.getWidth(), h = world.getHeight();
    unsigned int seed = world.getSeed();

    write(version);
    write(w);
    write(h);
    write(seed);

    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            const Tile& t = world.getTile(x, y);

            write(t.tileId);
            write(t.wallId);
            write(t.isActive);
        }
    }

    int activeSlot = inventory.activeSlot.load();
    write(activeSlot);

    for (const auto& slot : inventory.slots) {
        write(slot.itemId);
        write(slot.count);
    }

    std::cout << "[World] Saved binary to " << path << "\n";
}

bool WorldSerializer::load(World &world, Inventory &inventory) const {
    if (!fs::exists(path)) return false;

    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    auto read = [&](auto& value) {
        f.read(reinterpret_cast<char*>(&value), sizeof(value));
    };

    char magic[3];
    f.read(magic, 3);

    if (std::strncmp(magic, "TCW", 3) != 0) {
        std::cerr << "[World] Invalid save file\n";
        return false;
    }

    uint16_t version;
    read(version);

    if (version != 1) {
        std::cerr << "[World] Unsupported version: " << version << "\n";
        return false;
    }

    int w, h;
    unsigned int seed;

    read(w);
    read(h);
    read(seed);

    if (w != world.getWidth() || h != world.getHeight()) {
        std::cerr << "[World] Dimension mismatch\n";
        return false;
    }

    world.setSeed(seed);

    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            Tile &t = world.getTile(x, y);
            read(t.tileId);
            read(t.wallId);
            read(t.isActive);
        }
    }

    int activeSlot;
    read(activeSlot);

    inventory.activeSlot.store(activeSlot);

    for (auto& slot : inventory.slots) {
        read(slot.itemId);
        read(slot.count);
    }

    std::cout << "[World] Loaded binary from " << path << "\n";
    return true;
}
