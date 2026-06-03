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
    Game(GLFWwindow *window, VlkRenderer &renderer);

    Game(const Game &) = delete;

    Game &operator=(const Game &) = delete;

    ~Game();

    Player player;
    World world;

    void run();

    void onScroll(double yoffset);

    void saveWorld();
    void loadOrGenerateWorld();

    std::chrono::steady_clock::time_point lastAutoSave;

    // Threading
    std::thread meshThread;
    std::mutex meshQueueMutex;
    std::mutex meshResultMutex;
    std::condition_variable meshCV;
    std::vector<Chunk> meshQueue;
    std::vector<MeshResult> meshResults;
    std::atomic<bool> meshThreadRunning{true};

    void meshWorkerThread();

    // Lightmap threading
    std::thread lightmapThread;
    std::atomic<bool> lightmapThreadRunning{true};
    std::mutex lightmapMutex;
    std::condition_variable lightmapCV;
    std::atomic<bool> lightmapPending{false};
    std::atomic<bool> lightmapReady{false};

    struct LightmapBuffer {
        std::vector<uint8_t> pixels;
        int width = 0, height = 0;
        int originX = 0, originY = 0;
    };
    LightmapBuffer lightmapBack;

    struct LightmapRequest {
        int camTileX, camTileY, visX, visY;
    };
    LightmapRequest lightmapRequest;

    void lightmapWorkerThread();

private:
    InputState inputState;
    VlkRenderer &renderer;
    GLFWwindow *window;

    std::vector<Vertex> playerVertices;
    std::vector<uint32_t> playerIndices;

    std::vector<Vertex> inventoryVertices;
    std::vector<uint32_t> inventoryIndices;

    bool playerMeshDirty = true;
    bool inventoryMeshDirty = true;
    bool skyMeshDirty = true;
    bool lightmapDirty = true;

    glm::ivec2 lastLightmapCamTile = {-9999, -9999};

    std::vector<int64_t> cachedChunkKeys;

    std::vector<QuadSpec> meshScratch;

    std::atomic<bool> running{true};
    std::thread physicsThread;
    std::mutex worldMutex;
    std::mutex renderMutex;

    float mineTimer = 0.0f;
    glm::ivec2 miningTile = {-1, -1};

    WorldSerializer serializer;

    LightMap lightMap;

    CameraParams computeCameraParams(const Player &player, const World &world,
                                     int windowWidth, int windowHeight,
                                     float tileSize, float visibleTilesX);

    glm::ivec2 screenToTile(double mouseX, double mouseY, const CameraParams &cam,
                            int windowWidth, int windowHeight, float tileSize);

    void update(float deltaTime);

    void handleInput();

    void updateBuffers(const CameraParams &cam);

    void gameLoopThread();

    bool isChunkVisible(int chunkX, int chunkY, const CameraParams& cam) const;
};
