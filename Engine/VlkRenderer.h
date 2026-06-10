#pragma once

#include "../Include/Vertex.h"
#include "Include/CameraParams.h"

#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Lib/stb_truetype.h"
#include <vk_mem_alloc.h>

#include "stb_image.h"

/**
 * Core Vulkan rendering backend.
 *
 * Responsible for full GPU lifecycle management including:
 * - device selection and initialization
 * - swapchain and render pipeline creation
 * - GPU resource management (textures, buffers, descriptors)
 * - frame synchronization and command submission
 *
 * Acts as the single owner of all Vulkan objects used by the engine.
 */
class VlkRenderer {
public:
    VlkRenderer();

    // =========================================================================
    // Constants
    // =========================================================================

    const char** glfwExtensions;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const int MAX_FRAMES_IN_FLIGHT = 2;

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
        int useLighting;

        glm::vec2 skyUVOffset;
        glm::vec2 skyUVScale;

        glm::vec2 lightmapOrigin;
        glm::vec2 lightmapSize;

        int useSprite;
        int _pad[3];
    };

    struct MeshBuffer {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = VK_NULL_HANDLE;

        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = VK_NULL_HANDLE;

        uint32_t indexCount = 0;
    };

    struct TextureInfo {
        int width;
        int height;
        int channels;
        stbi_uc *pixels;
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

    std::unordered_map<std::string, MeshBuffer> meshes;       // player, inventory, sky, text
    std::unordered_map<int64_t, MeshBuffer> chunkMeshes;      // chunks only

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

    std::vector<VkImageView> swapChainImageViews;

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
    std::vector<VmaAllocation> uniformAllocations;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // =========================================================================
    // Main Texture Resources
    // =========================================================================

    VkImage textureImage;
    VmaAllocation textureImageAllocation;
    VkImageView textureImageView;
    VkSampler textureSampler;

    // =========================================================================
    // Depth Resources
    // =========================================================================

    VkImage depthImage;
    VmaAllocation depthImageAllocation;
    VkImageView depthImageView;

    // =========================================================================
    // Font Resources
    // =========================================================================

    VkImage fontImage;
    VmaAllocation fontImageAllocation;
    VkImageView fontImageView;
    VkSampler fontSampler;

    // =========================================================================
    // Sky Resources
    // =========================================================================

    VkImage skyImage = VK_NULL_HANDLE;
    VmaAllocation skyImageAllocation = VK_NULL_HANDLE;
    VkImageView skyImageView = VK_NULL_HANDLE;
    VkSampler skySampler = VK_NULL_HANDLE;

    // =========================================================================
    // Lightmap resources
    // =========================================================================

    VkImage       lightmapImage = VK_NULL_HANDLE;
    VmaAllocation lightmapAllocation = VK_NULL_HANDLE;
    VkImageView   lightmapImageView = VK_NULL_HANDLE;
    VkSampler     lightmapSampler = VK_NULL_HANDLE;

    int lightmapTexWidth = 0;
    int lightmapTexHeight = 0;

    glm::vec2 lastLightmapOrigin = { 0.0f, 0.0f };
    glm::vec2 lastLightmapSize = { 1.0f, 1.0f };

    // =========================================================================
    // Sprite Atlas Resources
    // =========================================================================

    VkImage spriteImage = VK_NULL_HANDLE;
    VmaAllocation spriteAllocation = VK_NULL_HANDLE;
    VkImageView spriteImageView = VK_NULL_HANDLE;
    VkSampler spriteSampler = VK_NULL_HANDLE;

    // =========================================================================
    // VMA Allocator
    // =========================================================================

    VmaAllocator vmaAllocator = VK_NULL_HANDLE;

    // =========================================================================
    // Persistent staging buffer
    // =========================================================================

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = VK_NULL_HANDLE;
    void* stagingMapped = nullptr;
    VkDeviceSize stagingBufferSize = 0;

    static constexpr VkDeviceSize STAGING_BUFFER_SIZE = 32 * 1024 * 1024; // 32MB

    // =========================================================================
    // Transfer batching
    // =========================================================================
    VkCommandPool transferCommandPool;
    VkCommandBuffer transferCommandBuffer;
    VkFence transferFence;
    bool transferOpen = false;

    VkDeviceSize stagingOffset = 0;

    // =========================================================================
    // Initialization / Setup
    // =========================================================================

    /**
     * Creates Vulkan instance (entry point for all GPU functionality).
     */
    void createInstance();

    /**
     * Creates window surface for presentation.
     *
     * @param window GLFW window used for rendering target
     */
    void createSurface(
        GLFWwindow* window
    );

    /**
     * Selects the physical GPU used for rendering.
     */
    void pickPhysicalDevice();

    /**
     * Creates logical device and queues (graphics + present).
     */
    void createLogicalDevice();

    /**
     * Initializes Vulkan Memory Allocator (VMA) system.
     */
    void createVmaAllocator();

    /**
     * Creates staging buffer used for CPU → GPU transfers.
     */
    void createStagingBuffer();

    /**
     * Creates swapchain for presenting rendered images.
     *
     * @param window GLFW window (used for surface capabilities)
     */
    void createSwapChain(
        GLFWwindow* window
    );

    /**
     * Creates image views for swapchain images.
     */
    void createImageViews();

    /**
    * Creates render pass defining framebuffer layout.
    */
    void createRenderPass();

    /**
    * Creates descriptor set layout (shader resource bindings).
    */
    void createDescriptorSetLayout();

    /**
     * Creates graphics pipelines (world + UI rendering).
     */
    void createGraphicsPipeline();

    /**
     * Creates framebuffers for swapchain images.
     */
    void createFramebuffers();

    /**
    * Creates command pool used for rendering command buffers.
    */
    void createCommandPool();

    /**
     * Creates transfer system resources for batched GPU uploads.
     */
    void createTransferResources();

    /**
     * Creates depth buffer used for depth testing in 3D rendering.
     */
    void createDepthResources();

    /**
     * Loads a texture from disk into GPU memory.
     *
     * @param path File path to texture image
     */
    void createTextureImage(
        const std::string& path
    );

    /**
     * Creates image view for main texture.
     */
    void createTextureImageView();

    /**
     * Creates sampler used for texture sampling in shaders.
     */
    void createSampler();

    /**
     * Allocates per-frame uniform buffers.
     */
    void createUniformBuffers();

    /**
     * Creates descriptor pool for GPU resource binding.
     */
    void createDescriptorPool();

    /**
     * Allocates descriptor sets for shader access.
     */
    void createDescriptorSets();

    /**
     * Allocates command buffers for rendering.
     */
    void createCommandBuffers();

    /**
     * Creates synchronization primitives (fences + semaphores).
     */
    void createSyncObjects();

    // =========================================================================
    // Frame Rendering
    // =========================================================================

    /**
     * Executes a full frame render.
     *
     * Handles:
     * - swapchain acquisition
     * - command buffer execution
     * - rendering world + UI
     * - presentation
     *
     * @param window GLFW window (used for surface / resize handling)
     * @param framebufferResized Flag set when swapchain must be recreated
     * @param cam Camera state used for view/projection matrices
     */
    void drawFrame(
        GLFWwindow* window,
        bool& framebufferResized,
        const CameraParams& cam
    );

    /**
     * Records rendering commands for a single swapchain image.
     *
     * @param commandBuffer Active command buffer for this frame
     * @param imageIndex Swapchain image index being rendered to
     */
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex
    );

    /**
     * Updates per-frame uniform buffer data.
     *
     * @param currentImage Frame index in flight
     * @param cam Camera used to compute view/projection matrices
     */
    void updateUniformBuffer(
        uint32_t currentImage,
        const CameraParams& cam
    );

    /**
     * Recreates swapchain and all dependent GPU resources.
     *
     * @param window GLFW window (used for new surface dimensions)
     */
    void recreateSwapChain(
        GLFWwindow* window
    );

    /**
     * Destroys all swapchain-dependent resources.
     */
    void cleanupSwapChain();

    // =========================================================================
    // Mesh Management
    // =========================================================================

    /**
     * Updates vertex buffer for a named mesh.
     *
     * Replaces GPU data for an existing mesh.
     *
     * @param name Mesh identifier (e.g. "player", "ui", "sky")
     * @param vertices New vertex data
     */
    void updateVertexBuffer(
        const std::string& name,
        const std::vector<Vertex>& vertices
    );

    /**
     * Updates index buffer for a named mesh.
     *
     * @param name Mesh identifier
     * @param indices New index data
     */
    void updateIndexBuffer(
        const std::string& name,
        const std::vector<uint32_t>& indices
    );

    /**
     * Deletes a mesh and frees GPU memory.
     */
    void destroyMesh(
        const std::string& name
    );

    /**
     * Sets active chunk mesh keys for world rendering.
     */
    void setChunkKeys(const std::vector<int64_t>& keys);

    // Overloads
    void updateVertexBuffer(
        int64_t key,
        const std::vector<Vertex> &vertices
    );

    void updateIndexBuffer(
        int64_t key,
        const std::vector<uint32_t> &indices
    );

    void destroyMesh(
        int64_t key
    );

    // =========================================================================
    // Drawing
    // =========================================================================

    /**
     * Renders a named mesh.
     */
    void draw(
        const std::string& name,
        VkCommandBuffer commandBuffer
    );

    // Overload
    /**
    * Renders a chunk mesh.
    */
    void draw(
        int64_t key,
        VkCommandBuffer commandBuffer
    );

    /**
     * Renders a UI mesh (screen-space).
     */
    void drawUI(
        const std::string& name,
        VkCommandBuffer commandBuffer
    );

    /**
     * Renders text buffer using SDF font system.
     */
    void drawText(
        VkCommandBuffer commandBuffer
    );

    /**
     * Renders sky background layer.
     */
    void drawSky(
        VkCommandBuffer commandBuffer
    );

    /**
     * Renders item sprites
     */
    void drawSprite(
        const std::string& name,
        VkCommandBuffer commandBuffer
    );

    // =========================================================================
    // Text / Font
    // =========================================================================

    /**
     * Creates SDF font atlas from font file.
     *
     * @param fontPath Path to .ttf font file
     * @param fontSize Pixel size of generated font
     */
    void createFontTexture(
        const std::string& fontPath,
        int fontSize
    );

    /**
     * Builds mesh for text rendering.
     *
     * @param calls List of text draw commands
     */
    void buildTextMesh(
        const std::vector<TextDrawCall>& calls
    );

    /**
     * Sets text draw queue for next frame.
     */
    void setTextDrawCalls(
        const std::vector<TextDrawCall>& calls
    );

    // =========================================================================
    // Sky
    // =========================================================================

    /**
     * Loads sky texture used for background rendering.
     */
    void createSkyTexture(const std::string& path);

    /**
     * Updates sky rendering offset based on camera movement.
     *
     * @param cam Camera used for parallax calculation
     * @param parallaxFactor Movement scaling factor
     */
    void updateSkyMesh(
        const CameraParams& cam,
        float parallaxFactor = 0.1f
    );

    // =========================================================================
    // Lightmap
    // =========================================================================

    /**
     * Creates GPU texture used for dynamic lighting.
     */
    void createLightmapTexture(
        int width,
        int height
    );

    /**
     * Uploads new lightmap data to GPU.
     *
     * @param pixels Raw lightmap pixel data (RGBA or grayscale)
     * @param width Texture width
     * @param height Texture height
     */
    void updateLightmap(
        const uint8_t* pixels,
        int width,
        int height
    );

    /**
     * Destroys lightmap GPU resources.
     */
    void destroyLightmap();

    // =========================================================================
    // Sprite Atlas
    // =========================================================================

    /**
     * Creates GPU texture atlas for the sprites
     *
     * @param pixels Raw texture pixel data
     * @param width Texture width
     * @param height Texture height
     */
    void createSpriteAtlas(
        const std::vector<uint8_t>& pixels,
        int width,
        int height
    );

    /**
     * Destroys sprite atlas GPU resources
     */
    void destroySpriteAtlas();

    /**
     * Helper to update sprite atlas descriptors
     */
    void updateSpriteAtlasDescriptors();

    // =========================================================================
    // Buffers / Images / Memory
    // =========================================================================

    /**
     * Creates a GPU buffer using VMA allocation.
     *
     * @param size Size of the buffer in bytes
     * @param usage Vulkan usage flags defining buffer role (vertex, index, transfer, etc.)
     * @param memUsage Memory usage hint (CPU-only, GPU-only, etc.)
     * @param buffer Output buffer handle
     * @param allocation Output VMA memory allocation handle
     */
    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memUsage,
        VkBuffer& buffer,
        VmaAllocation &allocation
    );

    /**
     * Copies data between GPU buffers.
     *
     * @param srcBuffer Source buffer (must be transfer source capable)
     * @param dstBuffer Destination buffer (must be transfer destination capable)
     * @param size Number of bytes to copy
     * @param srcOffset Offset into source buffer in bytes
     */
    void copyBuffer(
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size,
        VkDeviceSize srcOffset
    );

    /**
     * Destroys the staging buffer used for CPU → GPU transfers.
     */
    void destroyStagingBuffer();

    /**
     * Ensures staging buffer has at least the requested capacity.
     *
     * @param size Minimum required size in bytes
     */
    void ensureStagingBuffer(
        VkDeviceSize size
    );

    /**
     * Creates a GPU image resource.
     *
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param format Pixel format of the image
     * @param tiling Memory layout mode (linear or optimal)
     * @param usage Image usage flags (sampling, transfer, attachment, etc.)
     * @param memUsage Memory usage hint for allocation
     * @param image Output image handle
     * @param imageAllocation Output memory allocation handle
     */
    void createImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VmaMemoryUsage memUsage,
        VkImage& image,
        VmaAllocation& imageAllocation
    );

    /**
     * Copies buffer data into a GPU image.
     *
     * @param buffer Source buffer containing pixel data
     * @param image Destination image
     * @param width Image width in pixels
     * @param height Image height in pixels
     */
    void copyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    );

    /**
    * Transitions an image between GPU layouts.
    *
    * @param image Image to transition
    * @param format Image format (used for layout decisions)
    * @param oldLayout Current layout
    * @param newLayout Target layout
    */
    void transitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    );

    /**
     * Creates an image view for shader access.
     *
     * @param image Source image
     * @param format Image format
     * @param aspectFlags Which aspects of the image to expose (color, depth, etc.)
     * @return Created image view
     */
    VkImageView createImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags
    );

    /**
     * Begins a batched transfer sequence.
     * Used to group multiple GPU uploads into a single command buffer.
     */
    void beginTransferBatch();

    /**
     * Ends a batched transfer sequence and submits it to the GPU.
     */
    void endTransferBatch();

    /**
     * Blocks until all pending transfer operations are complete.
     */
    void waitTransferComplete();

    // =========================================================================
    // Device / Swapchain Queries
    // =========================================================================

    int rateDeviceSuitability(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    bool isDeviceSuitable(VkPhysicalDevice device);

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

    // =========================================================================
    // Cleanup
    // =========================================================================

    void cleanupMeshes();

private:

    // =========================================================================
    // Internal State
    // =========================================================================

    std::vector<int64_t> chunkKeys;

    std::vector<TextDrawCall> textDrawCalls;

    SDFFont sdfFont;

    // =========================================================================
    // Sampler creating helper
    // =========================================================================

    VkSampler makeSampler(
        const VkSamplerCreateInfo& info
    );

    VkSamplerCreateInfo defaultSamplerInfo();

    // =========================================================================
    // Frame rendering helper
    // =========================================================================

    void bindPipeline(
        VkCommandBuffer cmd,
        VkPipeline pipeline,
        const VkViewport &viewport,
        const VkRect2D &scissor
    );

    // =========================================================================
    // Texture creation helper
    // =========================================================================

    TextureInfo loadTextureImage(
        const std::string& path
    );

    void uploadTexture(
        const void* pixels,
        VkDeviceSize imageSize,
        VkImage image,
        VkFormat format,
        uint32_t width,
        uint32_t height
    );

    // =========================================================================
    // Mesh management helpers
    // =========================================================================

    template<typename Key>
    MeshBuffer& getMeshBuffer(
        std::unordered_map<Key, MeshBuffer>& map,
        const Key& key
    );

    template<typename T>
    void uploadToGpuBuffer(
        VkBuffer& buffer,
        VmaAllocation& allocation,
        const std::vector<T>& data,
        VkBufferUsageFlags useFlags
    );

    template<typename Key>
    void destroyMeshImpl(
        std::unordered_map<Key, MeshBuffer>& map,
        const Key& key
    );

    // =========================================================================
    // Drawing helpers
    // =========================================================================

    UIPushConstants makeOrthoPush();

    void drawMesh(
        const MeshBuffer& mesh,
        VkCommandBuffer commandBuffer
    );

    void drawWithPush(
        const std::string& name,
        VkCommandBuffer commandBuffer,
        const UIPushConstants& push
    );

};
