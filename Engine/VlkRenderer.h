#pragma once

#include "Vertex.h"
#include "../Include/CameraParams.h"

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Lib/stb_truetype.h"

class VlkRenderer {
public:
    // Variables
    uint32_t glfwExtensionCount = 0;
    uint32_t currentFrame = 0;
    uint32_t indexCount;

    std::array<stbtt_bakedchar, 96> bakedChars;
    int fontAtlasWidth = 512;
    int fontAtlasHeight = 512;

    // Constants
    const char **glfwExtensions;
    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Structs
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct UIPushConstants {
        glm::mat4 proj;
        int useUIProj;
        int useFont;
        int _pad[2];    // Needed for 16-byte alignment
    };

    struct MeshBuffer {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        uint32_t indexCount = 0;
    };

    struct TextDrawCall {
        std::string text;
        float x, y;
        glm::vec3 color;
    };

    std::unordered_map<std::string, MeshBuffer> meshes;

    // Vulkan Datatype Variables
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    static std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkImage fontImage;
    VkDeviceMemory fontImageMemory;
    VkImageView fontImageView;
    VkSampler fontSampler;

    // Methods
    void createInstance();

    void createSurface(GLFWwindow *window);

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain(GLFWwindow *window);

    void createImageViews();

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createDepthResources();

    void createTextureImage();

    void createTextureImageView();

    void createSampler();

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffers();

    void updateUniformBuffer(uint32_t currentImage, const CameraParams &cam);

    void updateVertexBuffer(const std::string &name, const std::vector<Vertex> &vertices);

    void updateIndexBuffer(const std::string &name, const std::vector<uint32_t> &indices);

    void destroyMesh(const std::string &name);

    void draw(const std::string &name, VkCommandBuffer commandBuffer);

    void drawUI(const std::string &name, VkCommandBuffer commandBuffer);

    void drawText(const std::string &text, float x, float y, glm::vec3 color, VkCommandBuffer commandBuffer);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void cleanupSwapChain();

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);

    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    void createImage(uint32_t width, uint32_t height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage &image,
                     VkDeviceMemory &imageMemory);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);

    void drawFrame(GLFWwindow *window, bool &framebufferResized, const CameraParams &cam);

    void recreateSwapChain(GLFWwindow *window);

    void createFontTexture(const std::string &fontPath, int fontSize);

    int rateDeviceSuitability(VkPhysicalDevice device);

    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    bool hasStencilComponent(VkFormat format);

    std::vector<const char *> getRequiredExtensions();

    static std::vector<char> readFile(const std::string &filename);

    static void framebufferResizeCallback(GLFWwindow *window, int width,
                                          int height);

    void setChunkKeys(const std::vector<std::string> &keys);

    // Vulkan Datatype Methods
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                                GLFWwindow *window);

    VkShaderModule createShaderModule(const std::vector<char> &code);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkImageView createImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectFlags);

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    void setTextDrawCalls(const std::vector<TextDrawCall> &calls);

private:
    std::vector<std::string> chunkKeys;
    std::vector<TextDrawCall> textDrawCalls;
};
