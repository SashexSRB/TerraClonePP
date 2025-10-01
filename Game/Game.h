#pragma once

#include "../Engine/VlkRenderer.h"
#include "Player.h"
#include "World.h"
#include <GLFW/glfw3.h>

struct CameraParams {
  glm::vec2 position;
  float visibleWidth;
  float visibleHeight;
};

class Game {
public:
  Game(GLFWwindow *window, VlkRenderer &renderer);
  Game(const Game &) = delete;
  Game &operator=(const Game &) = delete;
  Player player;
  World world;
  void run();
  void notifyWorldChanged();

private:
  VlkRenderer &renderer;
  GLFWwindow *window;
  bool worldChanged = true;

  std::vector<Vertex> worldVertices;
  std::vector<uint32_t> worldIndices;

  std::vector<Vertex> playerVertices;
  std::vector<uint32_t> playerIndices;

  std::vector<Vertex> inventoryVertices;
  std::vector<uint32_t> inventoryIndices;

  CameraParams computeCameraParams(const Player &player, const World &world,
                                   int windowWidth, int windowHeight,
                                   float tileSize, float visibleTilesX);

  glm::ivec2 screenToTile(double mouseX, double mouseY, const CameraParams &cam,
                          int windowWidth, int windowHeight, float tileSize);

  void update(float deltaTime);
  void handleInput();
  void updateBuffers();
};
