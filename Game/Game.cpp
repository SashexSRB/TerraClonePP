#include "Game.h"
#include "Constants.h"
#include "Entity/Physics.h"
#include "../VulkanApp.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <glm/gtc/matrix_transform.hpp>

VulkanApp app;

Game::Game(GLFWwindow *window, VlkRenderer &renderer)
    : renderer(renderer), window(window), world(8400, 2400), player() {
    player.position = {
        (world.getWidth() * 32.0f) / 2.0f - 32.0f,
        (world.getHeight() * 32.0f / 2.0f - 32.0f)
    };
    renderer.setGame(*this);

    // Generate a random seed using system clock
    unsigned int seed = static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count());

    world.generate(seed);

    CameraParams cam = computeCameraParams(
            player, world,
            renderer.swapChainExtent.width,
            renderer.swapChainExtent.height,
            Constants::TileSize, Constants::VisibleTilesX
    );

    updateBuffers(cam);

    std::cout << "[Game] Starting physics thread...\n";
    physicsThread = std::thread(&Game::gameLoopThread, this);
}

Game::~Game() {
    std::cout << "[Game] Stopping physics thread...\n";
    running = false;
    if (physicsThread.joinable())
        physicsThread.join();
    std::cout << "[Game] Physics thread stopped.\n";
}

void Game::gameLoopThread() {
    std::cout << "[Game] Thread started.\n";
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // lock the world while updating
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            update(deltaTime);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::cout << "[Game] Thread exiting.\n";
}

void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        CameraParams cam;
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            handleInput();
            cam = computeCameraParams(
                player, world,
                renderer.swapChainExtent.width,
                renderer.swapChainExtent.height,
                Constants::TileSize, Constants::VisibleTilesX
            );
            updateBuffers(cam);
        }

        {
            std::lock_guard<std::mutex> lock(renderMutex);
            renderer.drawFrame(window, app.framebufferResized, cam);
        }

        glfwPollEvents();
    }

    if (physicsThread.joinable()) {
        std::cout << "[Game] waiting for physics thread to exit...\n";
        physicsThread.join();
        std::cout << "[Game] Physics thread exited cleanly.\n";
    }
}

void Game::update(float deltaTime) {
    applyMovement(player, inputState, deltaTime);
    resolveCollisions(player, world, deltaTime);
}

CameraParams Game::computeCameraParams(const Player &player, const World &world,
                                       int windowWidth, int windowHeight,
                                       float tileSize, float visibleTilesX) {
    CameraParams cam;

    cam.visibleWidth = visibleTilesX * tileSize;
    cam.visibleHeight = cam.visibleWidth * windowHeight / windowWidth;

    cam.position = player.position;

    // Clamp camera to world bounds
    float worldWidth = world.getWidth() * tileSize;
    float worldHeight = world.getHeight() * tileSize;

    cam.position.x = std::max(
        cam.visibleWidth / 2.0f,
        std::min(cam.position.x, worldWidth - cam.visibleWidth / 2.0f)
    );

    cam.position.y = std::max(
        cam.visibleHeight / 2.0f,
        std::min(cam.position.y, worldHeight - cam.visibleHeight / 2.0f)
    );

    return cam;
}

glm::ivec2 Game::screenToTile(double mouseX, double mouseY,
                              const CameraParams &cam, int windowWidth,
                              int windowHeight, float tileSize) {
    // Convert screen pixels to NDC (-1..1)
    float ndcX = static_cast<float>(mouseX) / windowWidth * 2.0f - 1.0f;
    float ndcY = static_cast<float>(mouseY) / windowHeight * 2.0f - 1.0f;

    // Map NDC to world coordinates
    float worldX = cam.position.x - cam.visibleWidth  / 2.0f + (ndcX + 1.0f) / 2.0f * cam.visibleWidth;
    float worldY = cam.position.y - cam.visibleHeight / 2.0f + (ndcY + 1.0f) / 2.0f * cam.visibleHeight;

    // World -> tile indices
    int tileX = static_cast<int>(worldX / tileSize);
    int tileY = static_cast<int>(worldY / tileSize);

    // Clamp to world
    tileX = std::clamp(tileX, 0, world.getWidth() - 1);
    tileY = std::clamp(tileY, 0, world.getHeight() - 1);

    return glm::ivec2(tileX, tileY);
}

void Game::handleInput() {
    // Graceful shutdown on ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        std::cout << "[Game] Shutting down...\n";
        running = false;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    inputState.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    inputState.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    inputState.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    CameraParams cam = computeCameraParams(
        player, world,
        renderer.swapChainExtent.width,
        renderer.swapChainExtent.height,
        32.0f, 100.0f
    );

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    float moveSpeed = player.moveSpeed;
    player.velocity.x = 0.0f;

    glm::ivec2 tileCoords = screenToTile(
        xpos, ypos, cam,
        renderer.swapChainExtent.width,
        renderer.swapChainExtent.height,
        32.0f
    );

    // Slot selection 1-9 (0 for slot 10)
    for (int i = 0; i < 9; ++i) {
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS) player.inventory.activeSlot = i;
    }

    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) player.inventory.activeSlot = 9;

    // Left click: remove tile
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (tileCoords.x >= 0 && tileCoords.x < world.getWidth() &&
            tileCoords.y >= 0 && tileCoords.y < world.getHeight()) {
            Tile t = world.getTile(tileCoords.x, tileCoords.y);
            if (t.isActive && TileRegistry::tileTypes[t.tileId].isSolid) {
                player.inventory.addItem(t.tileId, 1);
                t.isActive = false;
                world.setTile(tileCoords.x, tileCoords.y, t);
            }
        }
    }

    // Right click: place tile
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (tileCoords.x >= 0 && tileCoords.x < world.getWidth() &&
            tileCoords.y >= 0 && tileCoords.y < world.getHeight()) {
            Tile t = world.getTile(tileCoords.x, tileCoords.y);
            int activeSlot = player.inventory.activeSlot;

            if (!t.isActive && player.inventory.hasItemInSlot(activeSlot)) {
                t.tileId = player.inventory.slots[activeSlot].tileId;
                t.isActive = true;
                world.setTile(tileCoords.x, tileCoords.y, t);
                player.inventory.removeItem(activeSlot);
            }
        }
    }
}

void Game::updateBuffers(const CameraParams &cam) {
    int playerTileX = static_cast<int>(player.position.x / 32.0f);
    int playerTileY = static_cast<int>(player.position.y / 32.0f);
    bool chunksChanged = world.updateChunks(playerTileX, playerTileY, 2);
    {
        std::lock_guard<std::mutex> lock(renderMutex);

        // Clean up GPU buffers for chunks that are no longer loaded
        if (chunksChanged) {
            std::unordered_set<std::string> validKeys;
            for (auto &kv : World::loadedChunks)
                validKeys.insert(World::chunkMeshKey(kv.second.chunkX, kv.second.chunkY));

            std::vector<std::string> toRemove;
            for (auto &kv : renderer.meshes) {
                if (kv.first == "player" || kv.first == "inventory") continue;
                if (!validKeys.count(kv.first))
                    toRemove.push_back(kv.first);
            }
            for (auto &key : toRemove)
                renderer.destroyMesh(key);
        }

        // Update only dirty chunks
        std::vector<Vertex> chunkVerts;
        std::vector<uint32_t> chunkIndices;
        for (auto &kv : World::loadedChunks) {
            Chunk &chunk = kv.second;
            if (!chunk.needsUpdate) continue;

            world.generateChunkVertices(chunk, chunkVerts, chunkIndices);
            std::string key = World::chunkMeshKey(chunk.chunkX, chunk.chunkY);

            if (chunkVerts.empty() || chunkIndices.empty()) {
                renderer.destroyMesh(key);
                chunk.needsUpdate = false;
                continue;
            }

            renderer.updateVertexBuffer(key, chunkVerts);
            renderer.updateIndexBuffer(key, chunkIndices);
            chunk.needsUpdate = false;
        }

        // Always rebuild player and inventory every frame
        generatePlayerVertices(player, playerVertices, playerIndices);
        renderer.updateVertexBuffer("player", playerVertices);
        renderer.updateIndexBuffer("player", playerIndices);

        generateInventoryVertices(player.inventory, inventoryVertices, inventoryIndices);
        if (!inventoryVertices.empty() && !inventoryIndices.empty()) {
            renderer.updateVertexBuffer("inventory", inventoryVertices);
            renderer.updateIndexBuffer("inventory", inventoryIndices);
        } else {
            renderer.destroyMesh("inventory");
        }
    }
    renderer.updateUniformBuffer(0, cam);
}
