// Game/Game.cpp
#include "Game.h"
#include "../VulkanApp.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
// #include <iostream>

VulkanApp app;

Game::Game(GLFWwindow *window, VlkRenderer &renderer)
    : renderer(renderer), window(window), world(1000, 500), player(),
      worldChanged(true) {
  player.position = {(world.getWidth() * 32.0f) / 2.0f - 32.0f,
                     (world.getHeight() * 32.0f / 2.0f - 32.0f)};
  renderer.setGame(*this);

  // Generate a random seed using system clock
  unsigned int seed = static_cast<unsigned int>(
      std::chrono::system_clock::now().time_since_epoch().count());

  world.generate(seed);
  updateBuffers();
}

void Game::notifyWorldChanged() { worldChanged = true; }

void Game::run() {
  static float lastTime = glfwGetTime();
  float currentTime = glfwGetTime();
  float deltaTime = currentTime - lastTime;
  lastTime = currentTime;
  update(deltaTime);
  handleInput();
  updateBuffers();
  renderer.drawFrame(window, app.framebufferResized);
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
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int tileX = static_cast<int>(
        (xpos + player.position.x - renderer.swapChainExtent.width / 2.0f) /
        32.0f);
    int tileY = static_cast<int>(
        (ypos + player.position.y - renderer.swapChainExtent.height / 2.0f) /
        32.0f);
    if (tileX >= 0 && tileX < world.getWidth() && tileY >= 0 &&
        tileY < world.getHeight()) {
      Tile &tile = world.getTile(tileX, tileY);
      if (tile.isActive && TileRegistry::tileTypes[tile.tileId].isSolid) {
        player.inventory.addItem(tile.tileId, 1);
        tile.isActive = false;
        notifyWorldChanged();
      }
    }
  }
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int tileX = static_cast<int>(
        (xpos + player.position.x - renderer.swapChainExtent.width / 2.0f) /
        32.0f);
    int tileY = static_cast<int>(
        (ypos + player.position.y - renderer.swapChainExtent.height / 2.0f) /
        32.0f);
    if (tileX >= 0 && tileX < world.getWidth() && tileY >= 0 &&
        tileY < world.getHeight()) {
      Tile &tile = world.getTile(tileX, tileY);
      if (!tile.isActive && !player.inventory.items.empty()) {
        tile.tileId = player.inventory.items[0].first;
        tile.isActive = true;
        player.inventory.items[0].second--;
        if (player.inventory.items[0].second <= 0) {
          player.inventory.items.erase(player.inventory.items.begin());
        }
        notifyWorldChanged();
      }
    }
  }
}

void Game::updateBuffers() {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
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
  //                          renderer.swapChainExtent.height,
  //                          inventoryVertices, inventoryIndices);
  //  renderer.updateVertexBuffer("inventory", inventoryVertices);
  //  renderer.updateIndexBuffer("inventory", inventoryIndices);

  renderer.updateUniformBuffer(0);
}
