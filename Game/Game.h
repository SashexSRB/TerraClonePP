#pragma once

#include "../Engine/VlkRenderer.h"
#include "Player.h"
#include "World.h"
#include "CameraParams.h"
#include <GLFW/glfw3.h>
#include <atomic>
#include <mutex>
#include <thread>

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

private:
    InputState inputState;
    VlkRenderer &renderer;
    GLFWwindow *window;

    std::vector<Vertex> playerVertices;
    std::vector<uint32_t> playerIndices;

    std::vector<Vertex> inventoryVertices;
    std::vector<uint32_t> inventoryIndices;

    std::atomic<bool> running{true};
    std::thread physicsThread;
    std::mutex worldMutex;
    std::mutex renderMutex;

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
