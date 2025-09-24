// Game/Game.cpp
#include "Game.h"
#include "../VulkanApp.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
// #include <iostream>

VulkanApp app;

Game::Game(GLFWwindow *window, VlkRenderer &renderer)
    : renderer(renderer), window(window), world(100, 50), player(),
      worldChanged(true) {
  player.position = {50.0 * 16.0, 24.0 * 16.0};

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
  // Player movement
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    player.velocity.x = -player.moveSpeed;
  } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    player.velocity.x = player.moveSpeed;
  } else {
    player.velocity.x = 0.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.isGrounded) {
    player.velocity.y = -player.jumpSpeed;
    player.isGrounded = false;
  }
  player.velocity.y += 800.0f * deltaTime; // Gravity

  // Calculate proposed new position
  glm::vec2 newPosition = player.position + player.velocity * deltaTime;

  // Convert player position to tile coordinates
  int playerTileX = static_cast<int>(player.position.x / 16.0f);
  int playerTileY = static_cast<int>(player.position.y / 16.0f);
  int newTileX = static_cast<int>(newPosition.x / 16.0f);
  int newTileY = static_cast<int>(newPosition.y / 16.0f);

  // Clamp tile coordinates to world bounds
  playerTileX = std::max(0, std::min(playerTileX, world.getWidth() - 1));
  playerTileY = std::max(0, std::min(playerTileY, world.getHeight() - 1));
  newTileX = std::max(0, std::min(newTileX, world.getWidth() - 1));
  newTileY = std::max(0, std::min(newTileY, world.getHeight() - 1));

  // Helper lambda to check if tile is solid (tile or wall);
  auto isTileSolid = [&](int x, int y) -> bool {
    if (x < 0 || x >= world.getWidth() || y < 0 || y >= world.getHeight()) {
      return true;
    }
    Tile &tile = world.getTile(x, y);
    return (tile.isActive && TileRegistry::tileTypes[tile.tileId].isSolid) ||
           (tile.wallId != 0 && TileRegistry::wallTypes[tile.wallId].isSolid);
  };

  // Vertical collision (falling or jumping)
  if (player.velocity.y > 0.0f) { // Falling
    // Check tile below the proposed position
    if (newTileY < world.getHeight() && isTileSolid(newTileX, newTileY)) {
      // Snap player to the top of tile;
      player.position.y = newTileY * 16.0f;
      player.velocity.y = 0.0f;
      player.isGrounded = true;
    } else {
      // No collision, allow movement;
      player.position.y = newPosition.y;
      player.isGrounded = false;
    }
  } else if (player.velocity.y < 0.0f) { // Jumping
    // Check tile above the proposed position
    int aboveTileY = newTileY - 1;
    if (aboveTileY >= 0 && isTileSolid(newTileX, aboveTileY)) {
      // Snap player to the bottom of the tile;
      player.position.y = (aboveTileY + 1) * 16.0f;
      player.velocity.y = 0.0f;
    } else {
      // No collision, allow movement
      player.position.y = newPosition.y;
    }
  } else {
    // Check if player is grounded (tile below is solid)
    int belowTileY = playerTileY + 1;
    player.isGrounded = (belowTileY < world.getHeight() &&
                         isTileSolid(playerTileX, belowTileY));
  }

  // Horizontal collision (moving left or right)
  if (player.velocity.x != 0.0f) {
    // Check tile in direction of movement
    int targetTileX = newTileX + (player.velocity.x > 0.0f ? 1 : -1);
    if (isTileSolid(targetTileX, playerTileY)) {
      // Snap player to the edge of tile
      player.position.x =
          (player.velocity.x > 0.0f ? targetTileX : targetTileX + 1) * 16.0f;
      player.velocity.x = 0.0f;
    } else {
      // No collision, allow movement
      player.position.x = newPosition.x;
    }
  }

  // Clamp player position to world bounds
  player.position.x = std::max(
      0.0f, std::min(player.position.x, world.getWidth() * 16.0f - 1.0f));
  player.position.y = std::max(
      0.0f, std::min(player.position.y, world.getHeight() * 16.0f - 1.0f));

  /*
  player.position += player.velocity * deltaTime;

  // Basic collision (ground at y=25 * 16)
  float groundY = static_cast<float>(world.getHeight()) / 2 * 16.0f;
  if (player.position.y > groundY) {
    player.position.y = groundY;
    player.velocity.y = 0.0f;
    player.isGrounded = true;
  }
  */
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
        16.0f);
    int tileY = static_cast<int>(
        (ypos + player.position.y - renderer.swapChainExtent.height / 2.0f) /
        16.0f);
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
        16.0f);
    int tileY = static_cast<int>(
        (ypos + player.position.y - renderer.swapChainExtent.height / 2.0f) /
        16.0f);
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
