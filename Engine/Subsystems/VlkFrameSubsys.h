#pragma once
#include "VlkTypes.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VlkRenderer;

class VlkFrameSubsys {
public:
    VlkRenderer* r = nullptr;

    // Sync objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence>     inFlightFences;

    // Command recording
    VkCommandPool                commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Uniform buffers
    std::vector<VkBuffer>      uniformBuffers;
    std::vector<VmaAllocation> uniformAllocations;
    std::vector<void*>         uniformBuffersMapped;

    // Descriptor pool and sets
    VkDescriptorPool             descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    /**
    * Creates command pool used for rendering command buffers.
    */
    void createCommandPool();

    /**
     * Allocates command buffers for rendering.
     */
    void createCommandBuffers();

    /**
     * Creates synchronization primitives (fences + semaphores).
     */
    void createSyncObjects();

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
     * Updates per-frame uniform buffer data.
     *
     * @param currentImage Frame index in flight
     * @param cam Camera used to compute view/projection matrices
     */
    void updateUniformBuffer(
        uint32_t currentImage,
        const CameraParams& cam
    );

private:
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
     * Prepares the pipeline to be drawn for the current frame.
     *
     * @param cmd Active command buffer for this frame
     * @param pipeline Pipeline to which the frame will be sent
     * @param viewport Viewport
     * @param scissor Scissor
     */
    void bindPipeline(
        VkCommandBuffer cmd,
        VkPipeline pipeline,
        const VkViewport& viewport,
        const VkRect2D& scissor
    );
};