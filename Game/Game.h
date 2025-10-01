#pragma once

#include "../Engine/VlkRenderer.h"
#include "Player.h"
#include "World.h"
#include <GLFW/glfw3.h>

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

  void update(float deltaTime);
  void handleInput();
  void updateBuffers();
};
