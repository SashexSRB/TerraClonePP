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

struct InputState {
    std::atomic<bool> left  = {false};
    std::atomic<bool> right = {false};
    std::atomic<bool> jump  = {false};
};

struct MeshResult {
    int64_t chunkKey;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Game {
public:

    // =========================================================================
    // Construction / Lifetime
    // =========================================================================

    Game(GLFWwindow* window, VlkRenderer& renderer);

    Game(const Game&) = delete;

    Game& operator=(const Game&) = delete;

    ~Game();

    // =========================================================================
    // Constants
    // =========================================================================

    const std::string SAVE_PATH = ASSET_PATH "/Saves/world.tcw";

    // =========================================================================
    // Core Game State
    // =========================================================================

    Player player;
    World world;

    // =========================================================================
    // Public API
    // =========================================================================

    void run();

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

    void saveWorld();

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

    CameraParams computeCameraParams(
        const Player& player,
        const World& world,
        int windowWidth,
        int windowHeight,
        float tileSize,
        float visibleTilesX
    );

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

    void update(
        float deltaTime
    );

    void handleInput();

    void updateBuffers(
        const CameraParams& cam
    );

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

    void lightmapWorkerThread();

    // =========================================================================
    // Visibility
    // =========================================================================

    bool isChunkVisible(
        int chunkX,
        int chunkY,
        const CameraParams& cam
    ) const;
};