#include "VulkanApp.h"
#include "Game/Game.h"
#include "Items/Item.h"

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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
    std::cout << "[GLFW] Window Initialized.\n";
}

void VulkanApp::initVulkan() {
    vlkRenderer.createInstance();
    vlkRenderer.initValidator();
    vlkRenderer.createSurface(window);
    vlkRenderer.pickPhysicalDevice();
    vlkRenderer.createLogicalDevice();
    vlkRenderer.createVmaAllocator();
    vlkRenderer.createStagingBuffer();
    vlkRenderer.createSwapChain(window);
    vlkRenderer.createImageViews();
    vlkRenderer.createRenderPass();
    vlkRenderer.createDescriptorSetLayout();
    vlkRenderer.createGraphicsPipeline();
    vlkRenderer.createCommandPool();
    vlkRenderer.createTransferResources();
    vlkRenderer.createDepthResources();
    vlkRenderer.createFramebuffers();
    vlkRenderer.createTextureImage(ASSET_PATH "Assets/Textures/textures.png");
    vlkRenderer.createTextureImageView();
    vlkRenderer.createSampler();
    vlkRenderer.createFontTexture(ASSET_PATH "Assets/Fonts/ANDYB.TTF", 20);
    vlkRenderer.createSkyTexture(ASSET_PATH "Assets/Textures/sky.png");
    vlkRenderer.createLightmapTexture(1, 1);

    {
        uint8_t white[4] = {255, 255, 255, 255};
        std::vector<uint8_t> placeholder(white, white + 4);
        vlkRenderer.createSpriteAtlas(placeholder, 1, 1);
    }

    vlkRenderer.createUniformBuffers();
    vlkRenderer.createDescriptorPool();
    vlkRenderer.createDescriptorSets();

    Registry::initialize();
    {
        SpriteAtlas tmpAtlas;
        for (auto& [id, item] : Registry::items)
            if (!item.spritePath.empty())
                tmpAtlas.add(id, item.spritePath);
        tmpAtlas.add(SPRITE_PLAYER, ASSET_PATH "Assets/Sprites/player.png");
        int aw = 0, ah = 0;
        auto pixels = tmpAtlas.build(aw, ah);
        vlkRenderer.createSpriteAtlas(pixels, aw, ah);
    }

    vlkRenderer.createCommandBuffers();
    vlkRenderer.createSyncObjects();

    uint8_t white[4] = { 255, 255, 255, 255 };
    vlkRenderer.updateLightmap(white, 1, 1);

    game = new Game(window, vlkRenderer);
}

void VulkanApp::mainLoop() {
    game->run();
    vkDeviceWaitIdle(vlkRenderer.device);
}

void VulkanApp::cleanup() {
    vlkRenderer.cleanupSwapChain();

    // Sky
    vkDestroySampler(vlkRenderer.device, vlkRenderer.skySampler, nullptr);
    vkDestroyImageView(vlkRenderer.device, vlkRenderer.skyImageView, nullptr);
    vmaDestroyImage(vlkRenderer.vmaAllocator, vlkRenderer.skyImage, vlkRenderer.skyImageAllocation);

    // Font
    vkDestroySampler(vlkRenderer.device, vlkRenderer.fontSampler, nullptr);
    vkDestroyImageView(vlkRenderer.device, vlkRenderer.fontImageView, nullptr);
    vmaDestroyImage(vlkRenderer.vmaAllocator, vlkRenderer.fontImage, vlkRenderer.fontImageAllocation);

    // Texture atlas
    vkDestroySampler(vlkRenderer.device, vlkRenderer.textureSampler, nullptr);
    vkDestroyImageView(vlkRenderer.device, vlkRenderer.textureImageView, nullptr);
    vmaDestroyImage(vlkRenderer.vmaAllocator, vlkRenderer.textureImage, vlkRenderer.textureImageAllocation);

    // Lightmap
    vlkRenderer.destroyLightmap();

    // Sprite atlas
    vlkRenderer.destroySpriteAtlas();

    // Uniform buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vmaUnmapMemory(vlkRenderer.vmaAllocator, vlkRenderer.uniformAllocations[i]);
        vmaDestroyBuffer(vlkRenderer.vmaAllocator, vlkRenderer.uniformBuffers[i], vlkRenderer.uniformAllocations[i]);
    }

    vkDestroyDescriptorPool(vlkRenderer.device, vlkRenderer.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vlkRenderer.device, vlkRenderer.descriptorSetLayout, nullptr);

    vkDestroyPipeline(vlkRenderer.device, vlkRenderer.uiPipeline, nullptr);
    vkDestroyPipeline(vlkRenderer.device, vlkRenderer.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vlkRenderer.device, vlkRenderer.pipelineLayout, nullptr);

    vkDestroyRenderPass(vlkRenderer.device, vlkRenderer.renderPass, nullptr);

    vkDestroyFence(vlkRenderer.device, vlkRenderer.transferFence, nullptr);
    vkDestroyCommandPool(vlkRenderer.device, vlkRenderer.transferCommandPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vlkRenderer.device, vlkRenderer.imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vlkRenderer.device, vlkRenderer.renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(vlkRenderer.device, vlkRenderer.inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vlkRenderer.device, vlkRenderer.commandPool, nullptr);

    vlkRenderer.destroyStagingBuffer();
    vlkRenderer.cleanupMeshes();
    vmaDestroyAllocator(vlkRenderer.vmaAllocator);

    vkDestroyDevice(vlkRenderer.device, nullptr);

    if (vlkRenderer.valSubsys.enableValidationLayers)
        vlkRenderer.valSubsys.DestroyDebugUtilsMessengerEXT(vlkRenderer.instance, vlkRenderer.valSubsys.debugMessenger, nullptr);

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
