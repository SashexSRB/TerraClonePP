#include "VulkanApp.h"
#include "Game/Game.h"

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VulkanApp::VulkanApp() : window(nullptr), game(nullptr) { }
VulkanApp::~VulkanApp() {}

void VulkanApp::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
};

void VulkanApp::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    auto app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    if (app->game) app->game->onScroll(yoffset);
}

void VulkanApp::initWindow() {
    if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "TerraClone", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW Window!");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    std::cout << "OK: Window Initialized.\n";
}

void VulkanApp::initVulkan() {
    vlkRenderer.createInstance();
    validator.setupDebugMessenger(vlkRenderer.instance);
    vlkRenderer.createSurface(window);
    vlkRenderer.pickPhysicalDevice();
    vlkRenderer.createLogicalDevice();
    vlkRenderer.createSwapChain(window);
    vlkRenderer.createImageViews();
    vlkRenderer.createRenderPass();
    vlkRenderer.createDescriptorSetLayout();
    vlkRenderer.createGraphicsPipeline();
    vlkRenderer.createCommandPool();
    vlkRenderer.createDepthResources();
    vlkRenderer.createFramebuffers();
    vlkRenderer.createTextureImage();
    vlkRenderer.createTextureImageView();
    vlkRenderer.createSampler();
    vlkRenderer.createUniformBuffers();
    vlkRenderer.createDescriptorPool();
    vlkRenderer.createDescriptorSets();
    vlkRenderer.createCommandBuffers();
    vlkRenderer.createSyncObjects();

    TileRegistry::initialize();

    game = new Game(window, vlkRenderer);
}

void VulkanApp::mainLoop() {
    game->run();
    vkDeviceWaitIdle(vlkRenderer.device);
}

void VulkanApp::cleanup() {
    vlkRenderer.cleanupSwapChain();

    vkDestroySampler(vlkRenderer.device, vlkRenderer.textureSampler, nullptr);

    vkDestroyImageView(vlkRenderer.device, vlkRenderer.textureImageView, nullptr);

    vkDestroyImage(vlkRenderer.device, vlkRenderer.textureImage, nullptr);
    vkFreeMemory(vlkRenderer.device, vlkRenderer.textureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(vlkRenderer.device, vlkRenderer.uniformBuffers[i], nullptr);
        vkFreeMemory(vlkRenderer.device, vlkRenderer.uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(vlkRenderer.device, vlkRenderer.descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(vlkRenderer.device, vlkRenderer.descriptorSetLayout, nullptr);

    vkDestroyPipeline(vlkRenderer.device, vlkRenderer.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vlkRenderer.device, vlkRenderer.pipelineLayout, nullptr);

    vkDestroyRenderPass(vlkRenderer.device, vlkRenderer.renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vlkRenderer.device, vlkRenderer.imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vlkRenderer.device, vlkRenderer.renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(vlkRenderer.device, vlkRenderer.inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vlkRenderer.device, vlkRenderer.commandPool, nullptr);

    vkDestroyDevice(vlkRenderer.device, nullptr);

    if (validator.enableValidationLayers)
        validator.DestroyDebugUtilsMessengerEXT(vlkRenderer.instance, validator.debugMessenger, nullptr);

    vkDestroySurfaceKHR(vlkRenderer.instance, vlkRenderer.surface, nullptr);

    vkDestroyInstance(vlkRenderer.instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void VulkanApp::run() {
    initWindow();
    initVulkan();
    mainLoop();
    delete game;
    game = nullptr;
    cleanup();
}
