#pragma once

#include "Vertex.h"
#include "../Include/CameraParams.h"

#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Lib/stb_truetype.h"

class VlkRenderer {
public:

    // =========================================================================
    // Constants
    // =========================================================================

    const char** glfwExtensions;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // =========================================================================
    // Runtime State
    // =========================================================================

    uint32_t glfwExtensionCount = 0;
    uint32_t currentFrame = 0;
    uint32_t indexCount;

    CameraParams lastCam;

    // =========================================================================
    // Font / UI State
    // =========================================================================

    std::array<stbtt_bakedchar, 96> bakedChars;

    int fontAtlasWidth  = 512;
    int fontAtlasHeight = 512;

    glm::vec2 skyUVOffset = {0, 0};

    float skyUVScaleX = 1.0f;
    float skyUVScaleY = 1.0f;

    // =========================================================================
    // Structs
    // =========================================================================

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() &&
                   presentFamily.has_value();
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
        int useSky;

        int _pad[1]; // Needed for 16-byte alignment

        glm::vec2 skyUVOffset;
        glm::vec2 skyUVScale;
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

    struct SDFGlyph {
        float u0, v0, u1, v1; // UV in atlas

        float xoff, yoff;
        float xadvance;

        int width, height;
    };

    struct SDFFont {
        VkImage image;
        VkImageView view;
        VkSampler sampler;

        int atlasWidth;
        int atlasHeight;

        std::array<SDFGlyph, 96> glyphs; // ASCII 32–127
    };

    // =========================================================================
    // Mesh Storage
    // =========================================================================

    std::unordered_map<std::string, MeshBuffer> meshes;

    // =========================================================================
    // Core Vulkan Objects
    // =========================================================================

    VkInstance instance;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;

    // =========================================================================
    // Swapchain
    // =========================================================================

    std::vector<VkImage> swapChainImages;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    static std::vector<VkImageView> swapChainImageViews;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    // =========================================================================
    // Rendering Pipeline
    // =========================================================================

    VkRenderPass renderPass;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;
    VkPipeline uiPipeline;

    // =========================================================================
    // Commands & Synchronization
    // =========================================================================

    VkCommandPool commandPool;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkFence> inFlightFences;

    // =========================================================================
    // Uniforms & Descriptors
    // =========================================================================

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // =========================================================================
    // Main Texture Resources
    // =========================================================================

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    VkImageView textureImageView;
    VkSampler textureSampler;

    // =========================================================================
    // Depth Resources
    // =========================================================================

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // =========================================================================
    // Font Resources
    // =========================================================================

    VkImage fontImage;
    VkDeviceMemory fontImageMemory;

    VkImageView fontImageView;
    VkSampler fontSampler;

    // =========================================================================
    // Sky Resources
    // =========================================================================

    VkImage skyImage = VK_NULL_HANDLE;
    VkDeviceMemory skyImageMemory = VK_NULL_HANDLE;

    VkImageView skyImageView = VK_NULL_HANDLE;
    VkSampler skySampler = VK_NULL_HANDLE;

    // =========================================================================
    // Initialization / Setup
    // =========================================================================

    void createInstance();

    void createSurface(GLFWwindow* window);

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain(GLFWwindow* window);

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

    void createSyncObjects();

    // =========================================================================
    // Frame Rendering
    // =========================================================================

    void drawFrame(
        GLFWwindow* window,
        bool& framebufferResized,
        const CameraParams& cam
    );

    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex
    );

    void updateUniformBuffer(
        uint32_t currentImage,
        const CameraParams& cam
    );

    void recreateSwapChain(GLFWwindow* window);

    void cleanupSwapChain();

    // =========================================================================
    // Mesh Management
    // =========================================================================

    void updateVertexBuffer(
        const std::string& name,
        const std::vector<Vertex>& vertices
    );

    void updateIndexBuffer(
        const std::string& name,
        const std::vector<uint32_t>& indices
    );

    void destroyMesh(const std::string& name);

    void setChunkKeys(const std::vector<std::string>& keys);

    // =========================================================================
    // Drawing
    // =========================================================================

    void draw(
        const std::string& name,
        VkCommandBuffer commandBuffer
    );

    void drawUI(
        const std::string& name,
        VkCommandBuffer commandBuffer
    );

    void drawText(VkCommandBuffer commandBuffer);

    void drawSky(VkCommandBuffer commandBuffer);

    // =========================================================================
    // Text / Font
    // =========================================================================

    void createFontTexture(
        const std::string& fontPath,
        int fontSize
    );

    void buildTextMesh(
        const std::vector<TextDrawCall>& calls
    );

    void setTextDrawCalls(
        const std::vector<TextDrawCall>& calls
    );

    // =========================================================================
    // Sky
    // =========================================================================

    void createSkyTexture(const std::string& path);

    void updateSkyMesh(
        const CameraParams& cam,
        float parallaxFactor = 0.1f
    );

    // =========================================================================
    // Buffers / Images / Memory
    // =========================================================================

    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    );

    void copyBuffer(
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size
    );

    void createImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory
    );

    void copyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    );

    void transitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    );

    VkImageView createImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags
    );

    // =========================================================================
    // Device / Swapchain Queries
    // =========================================================================

    int rateDeviceSuitability(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

    uint32_t findMemoryType(
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    );

    bool hasStencilComponent(VkFormat format);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(
        VkPhysicalDevice device
    );

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    );

    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    );

    VkExtent2D chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        GLFWwindow* window
    );

    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    VkFormat findDepthFormat();

    // =========================================================================
    // Commands / Utility
    // =========================================================================

    VkShaderModule createShaderModule(
        const std::vector<char>& code
    );

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(
        VkCommandBuffer commandBuffer
    );

    std::vector<const char*> getRequiredExtensions();

    static std::vector<char> readFile(
        const std::string& filename
    );

    static void framebufferResizeCallback(
        GLFWwindow* window,
        int width,
        int height
    );

private:

    // =========================================================================
    // Internal State
    // =========================================================================

    std::vector<std::string> chunkKeys;

    std::vector<TextDrawCall> textDrawCalls;

    SDFFont sdfFont;
};