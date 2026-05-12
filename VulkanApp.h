#pragma once

#include "Engine/VlkRenderer.h"
#include "Engine/VlkValidator.h"
#include "Game/Game.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VlkValidator;

class VulkanApp {
public:
    VulkanApp();

    ~VulkanApp();

    void run();

    bool framebufferResized = false;

private:
    void initVulkan();

    void mainLoop();

    void cleanup();

    void initWindow();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    GLFWwindow *window;
    VlkRenderer vlkRenderer;
    VlkValidator validator;
    Game *game;

    static constexpr uint32_t WIDTH           = 1600;
    static constexpr uint32_t HEIGHT          = 800;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};
