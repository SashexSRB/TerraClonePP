// Game/Game.cpp
#include "Game.h"
#include "../VulkanApp.h"
#include <algorithm>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
// #include <iostream>

VulkanApp app;

Game::Game(GLFWwindow *window, VlkRenderer &renderer)
    : renderer(renderer), window(window), world(8400, 2400), player(),
      worldChanged(true) {
  player.position = {(world.getWidth() * 32.0f) / 2.0f - 32.0f,
                     (world.getHeight() * 32.0f / 2.0f - 32.0f)};
  renderer.setGame(*this);

  // Generate a random seed using system clock
  unsigned int seed = static_cast<unsigned int>(
      std::chrono::system_clock::now().time_since_epoch().count());

  world.generate(seed);
  updateBuffers();

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
    float deltaTime =
        std::chrono::duration<float>(currentTime - lastTime).count();
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

void Game::notifyWorldChanged() { worldChanged = true; }

void Game::run() {
  static float lastTime = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    handleInput();

    {
      std::lock_guard<std::mutex> lock(worldMutex);
      updateBuffers();
    }

    renderer.drawFrame(window, app.framebufferResized);

    glfwPollEvents();
  }

  if (physicsThread.joinable()) {
    std::cout << "[Game] waiting for physics thread to exit...\n";
    physicsThread.join();
    std::cout << "[Game] Physics thread exited cleanly.\n";
  }
}

void Game::update(float deltaTime) {
  const float playerSize = 32.0f;

  // -----------------------------
  // Handle horizontal input
  // -----------------------------
  player.velocity.x = 0.0f;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    player.velocity.x = -player.moveSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    player.velocity.x = player.moveSpeed;
  }

  // -----------------------------
  // Handle jump
  // -----------------------------
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.isGrounded) {
    player.velocity.y = -player.jumpSpeed;
    player.isGrounded = false;
  }

  // -----------------------------
  // Apply gravity
  // -----------------------------
  player.velocity.y += 800.0f * deltaTime;

  // -----------------------------
  // Helper lambda
  // -----------------------------
  auto isTileSolid = [&](int x, int y) -> bool {
    if (x < 0 || x >= world.getWidth() || y < 0 || y >= world.getHeight()) {
      return true;
    }
    Tile &tile = world.getTile(x, y);
    return (tile.isActive && TileRegistry::tileTypes[tile.tileId].isSolid) ||
           (tile.wallId != 0 && TileRegistry::wallTypes[tile.wallId].isSolid);
  };

  // -----------------------------
  // Horizontal collision
  // -----------------------------
  glm::vec2 newPos = player.position;
  newPos.x += player.velocity.x * deltaTime;

  int topTileY = static_cast<int>(player.position.y / playerSize);
  int bottomTileY =
      static_cast<int>((player.position.y + playerSize - 1) / playerSize);

  if (player.velocity.x > 0.0f) { // moving right
    int rightTileX = static_cast<int>((newPos.x + playerSize - 1) / playerSize);
    bool collision = false;
    for (int y = topTileY; y <= bottomTileY; ++y) {
      if (isTileSolid(rightTileX, y)) {
        collision = true;
        break;
      }
    }
    if (collision) {
      newPos.x = rightTileX * playerSize - playerSize;
      player.velocity.x = 0.0f;
    }
  } else if (player.velocity.x < 0.0f) { // moving left
    int leftTileX = static_cast<int>(newPos.x / playerSize);
    bool collision = false;
    for (int y = topTileY; y <= bottomTileY; ++y) {
      if (isTileSolid(leftTileX, y)) {
        collision = true;
        break;
      }
    }
    if (collision) {
      newPos.x = (leftTileX + 1) * playerSize;
      player.velocity.x = 0.0f;
    }
  }
  player.position.x = newPos.x;

  // -----------------------------
  // Ground check to prevent jitter
  // -----------------------------
  int leftTileX = static_cast<int>(player.position.x / playerSize);
  int rightTileX =
      static_cast<int>((player.position.x + playerSize - 1) / playerSize);

  if (player.isGrounded && player.velocity.y >= 0.0f) {
    // Check if still on solid ground (use current position, with epsilon)
    float bottomY = player.position.y + playerSize +
                    0.001; // Small epsilon for FP precision
    int bottomTileY = static_cast<int>(bottomY / playerSize);
    bool onGround = false;
    for (int x = leftTileX; x <= rightTileX; ++x) {
      if (isTileSolid(x, bottomTileY)) {
        onGround = true;
        break;
      }
    }
    if (onGround) {
      player.velocity.y = 0.0f;
    } else {
      player.isGrounded = false; // Ground disappeared, allow fall
    }
  }

  // -----------------------------
  // Vertical collision (stable)
  // -----------------------------
  newPos.y = player.position.y + player.velocity.y * deltaTime;

  if (player.velocity.y > 0.0f) { // falling
    int bottomTileY =
        static_cast<int>((newPos.y + playerSize - 1) / playerSize);
    bool collision = false;
    for (int x = leftTileX; x <= rightTileX; ++x) {
      if (isTileSolid(x, bottomTileY)) {
        collision = true;
        break;
      }
    }
    if (collision) {
      // Snap exactly on top of the tile
      newPos.y = bottomTileY * playerSize - playerSize;
      player.velocity.y = 0.0f;
      player.isGrounded = true;
    } else {
      player.isGrounded = false;
      player.position.y = newPos.y;
    }
  } else if (player.velocity.y < 0.0f) { // jumping
    int topTileYCheck = static_cast<int>(newPos.y / playerSize);
    bool collision = false;
    for (int x = leftTileX; x <= rightTileX; ++x) {
      if (isTileSolid(x, topTileYCheck)) {
        collision = true;
        break;
      }
    }
    if (collision) {
      // Snap below the ceiling
      newPos.y = (topTileYCheck + 1) * playerSize;
      player.velocity.y = 0.0f;
    } else {
      player.position.y = newPos.y;
    }
  } else {
    // Do nothing if velocity.y == 0
    // grounded state already handled during falling
  }
  player.position.y = newPos.y;

  // -----------------------------
  // Clamp to world bounds
  // -----------------------------
  player.position.x =
      std::max(0.0f, std::min(player.position.x,
                              world.getWidth() * playerSize - playerSize));
  player.position.y =
      std::max(0.0f, std::min(player.position.y,
                              world.getHeight() * playerSize - playerSize));
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
  cam.position.x =
      std::max(cam.visibleWidth / 2.0f,
               std::min(cam.position.x, worldWidth - cam.visibleWidth / 2.0f));
  cam.position.y = std::max(
      cam.visibleHeight / 2.0f,
      std::min(cam.position.y, worldHeight - cam.visibleHeight / 2.0f));

  return cam;
}

glm::ivec2 Game::screenToTile(double mouseX, double mouseY,
                              const CameraParams &cam, int windowWidth,
                              int windowHeight, float tileSize) {
  // Convert screen pixels to NDC (-1..1)
  float ndcX = static_cast<float>(mouseX) / windowWidth * 2.0f - 1.0f;
  float ndcY = static_cast<float>(mouseY) / windowHeight * 2.0f - 1.0f;

  // Map NDC to world coordinates
  float worldX = cam.position.x - cam.visibleWidth / 2.0f +
                 (ndcX + 1.0f) / 2.0f * cam.visibleWidth;
  float worldY = cam.position.y - cam.visibleHeight / 2.0f +
                 (ndcY + 1.0f) / 2.0f * cam.visibleHeight;

  // World -> tile indices
  int tileX = static_cast<int>(worldX / tileSize);
  int tileY = static_cast<int>(worldY / tileSize);

  // Clamp to world
  tileX = std::clamp(tileX, 0, world.getWidth() - 1);
  tileY = std::clamp(tileY, 0, world.getHeight() - 1);

  return glm::ivec2(tileX, tileY);
}

void Game::handleInput() {
  float moveSpeed = player.moveSpeed;
  player.velocity.x = 0.0f;

  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    player.velocity.x = -moveSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    player.velocity.x = moveSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.isGrounded) {
    player.velocity.y = -player.jumpSpeed;
    player.isGrounded = false;
  }

  // Graceful shutdown on ESC
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    std::cout << "[Game] Shutting down...\n";
    running = false;
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }

  CameraParams cam =
      computeCameraParams(player, world, renderer.swapChainExtent.width,
                          renderer.swapChainExtent.height, 32.0f, 100.0f);

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  glm::ivec2 tileCoords =
      screenToTile(xpos, ypos, cam, renderer.swapChainExtent.width,
                   renderer.swapChainExtent.height, 32.0f);

  // Left click: remove tile
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    if (tileCoords.x >= 0 && tileCoords.x < world.getWidth() &&
        tileCoords.y >= 0 && tileCoords.y < world.getHeight()) {

      Tile &tile = world.getTile(tileCoords.x, tileCoords.y);
      if (tile.isActive && TileRegistry::tileTypes[tile.tileId].isSolid) {
        player.inventory.addItem(tile.tileId, 1);
        tile.isActive = false;
        notifyWorldChanged();
      }
    }
  }

  // Right click: place tile
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    if (tileCoords.x >= 0 && tileCoords.x < world.getWidth() &&
        tileCoords.y >= 0 && tileCoords.y < world.getHeight()) {

      Tile &tile = world.getTile(tileCoords.x, tileCoords.y);
      if (!tile.isActive && !player.inventory.items.empty()) {
        tile.tileId = player.inventory.items[0].first;
        tile.isActive = true;
        player.inventory.items[0].second--;
        if (player.inventory.items[0].second <= 0)
          player.inventory.items.erase(player.inventory.items.begin());

        notifyWorldChanged();
      }
    }
  }
}

void Game::updateBuffers() {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  int playerTileX = static_cast<int>(player.position.x / 32.0f);
  int playerTileY = static_cast<int>(player.position.y / 32.0f);

  world.updateChunks(playerTileX, playerTileY, 2);

  if (worldChanged) {
    world.generateVertices(worldVertices, worldIndices);
    renderer.updateVertexBuffer("world", worldVertices);
    renderer.updateIndexBuffer("world", worldIndices);
    worldChanged = false;
    // std::cout << "World updated: " << worldVertices.size() << " vertices, "
    //          << worldIndices.size() << " indices\n";
  }

  generatePlayerVertices(player, playerVertices, playerIndices);
  renderer.updateVertexBuffer("player", playerVertices);
  renderer.updateIndexBuffer("player", playerIndices);

  // generateInventoryVertices(player.inventory, renderer.swapChainExtent.width,
  //                         renderer.swapChainExtent.height, inventoryVertices,
  //                          inventoryIndices);
  // renderer.updateVertexBuffer("inventory",
  // inventoryVertices);
  // renderer.updateIndexBuffer("inventory",
  // inventoryIndices);

  renderer.updateUniformBuffer(0);
}
