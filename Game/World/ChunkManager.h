#pragma once

#include "Chunk.h"
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

class World;

/**
 * Manages loading, unloading, and tracking of world chunks around the player.
 *
 * Responsible for maintaining the set of currently active chunks and
 * coordinating updates based on player position.
 */
class ChunkManager {
public:

    /**
     * @param world Reference to owning world (not owned by ChunkManager)
     * @param chunkSize Size of a single chunk in world tiles
     */
    ChunkManager(World& world, int chunkSize);

    /**
     * Updates loaded chunks based on player position.
     *
     * Loads/unloads chunks depending on the load radius.
     *
     * @param playerX Player X position in world coordinates
     * @param playerY Player Y position in world coordinates
     * @param loadRadiusChunks Radius (in chunks) around player to keep loaded
     *
     * @return True if chunk set changed (load/unload occurred)
     */
    bool update(int playerX, int playerY, int loadRadiusChunks);

    /**
     * Marks a world tile as dirty, triggering chunk rebuild if needed.
     *
     * @param worldX World-space X coordinate
     * @param worldY World-space Y coordinate
     */
    void markDirty(int worldX, int worldY);

    std::unordered_map<int64_t, Chunk>& getChunks() { return loadedChunks; }
    const std::unordered_map<int64_t, Chunk>& getChunks() const { return loadedChunks; }
    int getChunkSize() const { return chunkSize; }

    /**
     * Converts chunk coordinates into a unique 64-bit key.
     */
    static int64_t chunkKey(int cx, int cy);

    /**
     * Computes which chunk the player is currently inside.
     *
     * @param playerX Player world X coordinate
     * @param playerY Player world Y coordinate
     *
     * @return Chunk coordinates containing player position
     */
    glm::ivec2 playerChunk(int playerX, int playerY) const;

    /**
     * Generates a string key used for mesh identification/debugging.
     */
    static std::string meshKey(int cx, int cy);

    std::vector<int64_t> lastRemovedKeys;

private:
    World&     world;
    int        chunkSize;
    glm::ivec2 lastPlayerChunk = {-9999, -9999};
    std::unordered_map<int64_t, Chunk> loadedChunks;
};