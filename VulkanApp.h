#pragma once

#include "Engine/VlkRenderer.h"
#include "Engine/VlkValidator.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VlkValidator;
class Game;

/**
 * @brief Handles initialization, main loop, and cleanup of the Vulkan application.
 */
class VulkanApp {
public:
    VulkanApp();
    ~VulkanApp();

    /**
     * @brief First-called function in the program. Handles the initialization, main loop, cleanup and the GLFW init.
     */
    void run();

    /**
     * @brief Flag set when the framebuffer is resized.
     *
     * Used to trigger swapchain recreation.
     */
    bool framebufferResized = false;

private:
    /**
     * @brief Initializes the full Vulkan renderer stack.
     */
    void initVulkan();

    /**
     * @brief Main game loop.
     */
    void mainLoop();

    /**
     * Is called after the main game loop to free up resources and end Vulkan and GLFW.
     */
    void cleanup();

    /**
     * Initializes a GLFW instance.
     */
    void initWindow();

    /**
     * @brief Callback function that triggers a redraw when a window is resized.
     *
     * @param window The pointer to the GLFW instance.
     * @param width Window width.
     * @param height Window height.
     */
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    /**
     * @brief Handles scroll input events.
     *
     * @param window GLFW window pointer.
     * @param xoffset Scroll offset in x direction.
     * @param yoffset Scroll offset in y direction.
     */
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    /**
     * @brief Pointer to the GLFW Window instance.
     */
    GLFWwindow *window;

    /**
     * @brief Vulkan Renderer.
     */
    VlkRenderer vlkRenderer;

    /**
     * @brief Vulkan Validation member for printing errors and warnings.
     */
    VlkValidator validator;

    /**
     * @brief Pointer to the game instance.
     *
     * Owned externally. VulkanApp does not manage lifetime.
     */
    Game *game;

    /**
     * @brief Initial window width.
     */
    static constexpr uint32_t WIDTH           = 1600;

    /**
     * @brief Initial window height.
     */
    static constexpr uint32_t HEIGHT          = 800;

    /**
     * @brief Max amount of frames in flight.
     */
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};
