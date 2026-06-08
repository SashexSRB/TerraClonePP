#pragma once

#include "Entity/Player.h"
#include "World/World.h"
#include "World/LightMap.h"
#include "Engine/VlkRenderer.h"
#include "Include/CameraParams.h"
#include "World/WorldSerializer.h"

#include <GLFW/glfw3.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>

#include "Rendering/MeshUtils.h"

/**
 * @brief Represents current input state for gameplay actions.
 */
struct InputState {
    std::atomic<bool> left  = {false};
    std::atomic<bool> right = {false};
    std::atomic<bool> jump  = {false};
};

/**
 * Result of asynchronous mesh generation for a chunk.
 */
struct MeshResult {
    int64_t chunkKey;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

/**
 * @brief Core game class responsible for:
 * - Player and world management
 * - Game loop execution
 * - Input handling
 * - Mesh generation coordination
 * - Lightmap updates
 * - Saving/loading world state
 *
 * This class acts as the central gameplay controller.
 */
class Game {
public:

    // =========================================================================
    // Construction / Lifetime
    // =========================================================================

    /**
     * @brief Creates a Game instance.
     *
     * @param window GLFW window handle used for input and context access.
     * @param renderer Reference to Vulkan renderer.
     */
    Game(GLFWwindow* window, VlkRenderer& renderer);

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /**
     * Cleans up game systems and worker threads.
     */
    ~Game();

    // =========================================================================
    // Constants
    // =========================================================================

    /**
     * @brief Path used to save and load world data.
     */
    const std::string SAVE_PATH = ASSET_PATH "/Saves/world.tcw";

    // =========================================================================
    // Core Game State
    // =========================================================================

    Player player;
    World world;

    // =========================================================================
    // Public API
    // =========================================================================

    /**
     * @brief Starts the game loop.
     */
    void run();

    /**
     * @brief Handles mouse scroll input.
     *
     * @param yoffset Scroll direction/amount.
     */
    void onScroll(
        double yoffset
    );

private:

    // =========================================================================
    // Engine References
    // =========================================================================

    VlkRenderer& renderer;

    GLFWwindow* window;

    // =========================================================================
    // Input
    // =========================================================================

    InputState inputState;

    // =========================================================================
    // Render Mesh Data
    // =========================================================================

    std::vector<Vertex> playerVertices;
    std::vector<uint32_t> playerIndices;

    std::vector<Vertex> inventoryVertices;
    std::vector<uint32_t> inventoryIndices;

    std::vector<QuadSpec> meshScratch;

    // =========================================================================
    // Dirty Flags
    // =========================================================================

    bool playerMeshDirty = true;

    bool inventoryMeshDirty = true;

    bool skyMeshDirty = true;

    bool lightmapDirty = true;

    // =========================================================================
    // Chunk Rendering State
    // =========================================================================

    std::vector<int64_t> cachedChunkKeys;

    // =========================================================================
    // Lightmap State
    // =========================================================================

    glm::ivec2 lastLightmapCamTile = {-9999, -9999};

    LightMap lightMap;

    // =========================================================================
    // Gameplay State
    // =========================================================================

    float mineTimer = 0.0f;

    glm::ivec2 miningTile = {-1, -1};

    std::atomic<bool> running{true};

    std::chrono::steady_clock::time_point lastAutoSave;

    // =========================================================================
    // Serialization
    // =========================================================================

    WorldSerializer serializer;

    /**
     * Saves the current world to disk.
     */
    void saveWorld();

    /**
     * Loads world from disk or generates a new one if none exists.
     */
    void loadOrGenerateWorld();

    // =========================================================================
    // Main Threading
    // =========================================================================

    std::thread physicsThread;

    std::mutex worldMutex;

    std::mutex renderMutex;

    // =========================================================================
    // Camera Helpers
    // =========================================================================

    /**
     * @brief Computes camera parameters based on player and world state.
     *
     * @param player Current player state
     * @param world Current world instance
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     * @param tileSize Size of a single world tile in world units
     * @param visibleTilesX Number of tiles visible horizontally
     *
     * @return Computed camera parameters used for rendering
     */
    CameraParams computeCameraParams(
        const Player& player,
        const World& world,
        int windowWidth,
        int windowHeight,
        float tileSize,
        float visibleTilesX
    );

    /**
     * @brief Converts screen-space coordinates to world tile coordinates.
     *
     * @param mouseX Mouse X position in screen space
     * @param mouseY Mouse Y position in screen space
     * @param cam Active camera parameters
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     * @param tileSize Size of a world tile in world units
     *
     * @return Tile coordinate under the cursor
     */
    glm::ivec2 screenToTile(
        double mouseX,
        double mouseY,
        const CameraParams& cam,
        int windowWidth,
        int windowHeight,
        float tileSize
    );

    // =========================================================================
    // Game Loop
    // =========================================================================

    /**
     * @brief Updates game simulation state.
     *
     * @param deltaTime Time elapsed since last frame in seconds
     */
    void update(
        float deltaTime
    );

    /**
     * @brief Processes player input.
     */
    void handleInput();

    /**
     * @brief Updates GPU buffers every frame.
     *
     * @param cam Active camera parameters used to calculate positions, and are passed onto the renderer
     */
    void updateBuffers(
        const CameraParams& cam
    );

    /**
     * @brief Main game loop thread function.
     */
    void gameLoopThread();

    // =========================================================================
    // Mesh Generation Thread
    // =========================================================================

    std::thread meshThread;

    std::mutex meshQueueMutex;
    std::mutex meshResultMutex;

    std::condition_variable meshCV;

    std::vector<Chunk> meshQueue;
    std::vector<MeshResult> meshResults;

    std::atomic<bool> meshThreadRunning{true};

    /**
     * @brief Worker thread that generates chunk meshes asynchronously.
     */
    void meshWorkerThread();

    // =========================================================================
    // Lightmap Thread
    // =========================================================================

    struct LightmapBuffer {
        std::vector<uint8_t> pixels;
        int width = 0;
        int height = 0;
        int originX = 0;
        int originY = 0;
    };

    struct LightmapRequest {
        int camTileX;
        int camTileY;
        int visX;
        int visY;
    };

    std::thread lightmapThread;

    std::atomic<bool> lightmapThreadRunning{true};

    std::atomic<bool> lightmapPending{false};
    std::atomic<bool> lightmapReady{false};

    std::mutex lightmapMutex;

    std::condition_variable lightmapCV;

    LightmapBuffer lightmapBack;

    LightmapRequest lightmapRequest;

    /**
     * @brief Worker thread that updates lightmap data.
     */
    void lightmapWorkerThread();

    // =========================================================================
    // Visibility
    // =========================================================================

    /**
     * @brief Determines whether a chunk is within the camera's visible area.
     *
     * @param chunkX Chunk X coordinate
     * @param chunkY Chunk Y coordinate
     * @param cam Active camera parameters
     *
     * @return True if chunk is visible, false otherwise
     */
    bool isChunkVisible(
        int chunkX,
        int chunkY,
        const CameraParams& cam
    ) const;
};