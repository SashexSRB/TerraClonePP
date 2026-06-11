#pragma once

#include "VlkTypes.h"

class VlkRenderer;

class VlkTexSubsys {
public:
    VlkRenderer* r = nullptr;

    // Font Resources
    VkImage fontImage;
    VmaAllocation fontImageAllocation;
    VkImageView fontImageView;
    VkSampler fontSampler;

    std::vector<TextDrawCall> textDrawCalls;
    SDFFont sdfFont;

    // Sky Resources
    VkImage skyImage = VK_NULL_HANDLE;
    VmaAllocation skyImageAllocation = VK_NULL_HANDLE;
    VkImageView skyImageView = VK_NULL_HANDLE;
    VkSampler skySampler = VK_NULL_HANDLE;

    glm::vec2 skyUVOffset = {0, 0};

    float skyUVScaleX = 1.0f;
    float skyUVScaleY = 1.0f;

    // Lightmap resources
    VkImage       lightmapImage = VK_NULL_HANDLE;
    VmaAllocation lightmapAllocation = VK_NULL_HANDLE;
    VkImageView   lightmapImageView = VK_NULL_HANDLE;
    VkSampler     lightmapSampler = VK_NULL_HANDLE;

    int lightmapTexWidth = 0;
    int lightmapTexHeight = 0;
    glm::vec2 lastLightmapOrigin = { 0.0f, 0.0f };
    glm::vec2 lastLightmapSize = { 1.0f, 1.0f };

    // Sprite Atlas Resources
    VkImage spriteImage = VK_NULL_HANDLE;
    VmaAllocation spriteAllocation = VK_NULL_HANDLE;
    VkImageView spriteImageView = VK_NULL_HANDLE;
    VkSampler spriteSampler = VK_NULL_HANDLE;


    // ===================================================
    // Text
    // ===================================================
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
     *
     * @param calls List of text draw commands
     */
    void setTextDrawCalls(
        const std::vector<TextDrawCall>& calls
    );

    // ===================================================
    // Sky
    // ===================================================

    /**
     * Loads sky texture used for background rendering.
     *
     * @param path Texture source path
     */
    void createSkyTexture(
        const std::string& path
    );

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

    // ===================================================
    // Lightmap
    // ===================================================

    /**
     * Creates GPU texture used for dynamic lighting.
     *
     * @param width Lightmap texture width
     * @param height Lightmap texture height
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

    // ===================================================
    // Sprite Atlas
    // ===================================================

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

    // ===================================================
    // Helpers
    // ===================================================
    /**
     * Creates a texture sampler to use for textures.
     *
     * @param info Sampler info struct for sampler creation
     * @return Created texture sampler.
     */
    VkSampler makeSampler(
        const VkSamplerCreateInfo& info
    );

    /**
     * Populates default texture sampler struct
     * @return Populated sampler
     */
    VkSamplerCreateInfo defaultSamplerInfo();

    /**
     * Loads a texture from an image file (.png)
     *
     * Texture dimensions must be divisible by 8.
     *
     * @param path Path to the texture image.
     * @return Returns the information on texture in a struct, containing width, height, channels, and raw pixel data
     */
    TextureInfo loadTextureImage(
        const std::string& path
    );

    /**
     * Uploads the created texture to the GPU.
     *
     * @param pixels Raw pixel data
     * @param imageSize Size of the image
     * @param image Image object itself
     * @param format Format used for the image
     * @param width Image width
     * @param height Image height
     */
    void uploadTexture(
        const void* pixels,
        VkDeviceSize imageSize,
        VkImage image,
        VkFormat format,
        uint32_t width,
        uint32_t height
    );
};