#pragma once

#include "Entity/Player.h"
#include "World/World.h"
#include "Engine/VlkRenderer.h"
#include "Include/CameraParams.h"
#include "World/WorldSerializer.h"

#include <GLFW/glfw3.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

#include "Rendering/MeshUtils.h"

struct InputState {
    std::atomic<bool> left  = {false};
    std::atomic<bool> right = {false};
    std::atomic<bool> jump  = {false};
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
    std::vector<std::string> cachedChunkKeys;

    std::vector<QuadSpec> meshScratch;

    std::atomic<bool> running{true};
    std::thread physicsThread;
    std::mutex worldMutex;
    std::mutex renderMutex;

    float mineTimer = 0.0f;
    glm::ivec2 miningTile = {-1, -1};

    WorldSerializer serializer;

    CameraParams computeCameraParams(const Player &player, const World &world,
                                     int windowWidth, int windowHeight,
                                     float tileSize, float visibleTilesX);

    glm::ivec2 screenToTile(double mouseX, double mouseY, const CameraParams &cam,
                            int windowWidth, int windowHeight, float tileSize);

    void update(float deltaTime);

    void handleInput();

    void updateBuffers(const CameraParams &cam);

    void gameLoopThread();
};
