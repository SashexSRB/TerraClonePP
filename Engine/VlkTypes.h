#pragma once
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <unordered_map>

#define STBI_IMPLEMENTATION
#include "Lib/stb_image.h"
#include "Lib/stb_truetype.h"

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

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

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

#include "Include/CameraParams.h"
#include "Include/Vertex.h"