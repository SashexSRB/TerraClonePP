#pragma once
#include "VlkTypes.h"

class VlkRenderer;

class VlkDrawSubsys {
public:
    VlkRenderer* r = nullptr;

    /**
     * Renders a named mesh.
     */
    void draw(
        const std::string& name,
        VkCommandBuffer commandBuffer
    );

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

private:

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