#include "VlkTexSubsys.h"
#include "VlkRenderer.h"

#include "stb_image.h"
#include "stb_truetype.h"

#include <fstream>
#include <iostream>
#include <cstring>

// =========================================================================
// Text / Font
// =========================================================================

void VlkTexSubsys::createFontTexture(const std::string& fontPath, int fontSize) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open font");
    size_t size = file.tellg();
    file.seekg(0);
    std::vector<unsigned char> fontBuffer(size);
    file.read((char*)fontBuffer.data(), size);
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(),
        stbtt_GetFontOffsetForIndex(fontBuffer.data(), 0)))
        throw std::runtime_error("Font init failed");

    const int W = 512;
    const int H = 512;
    std::vector<unsigned char> atlas(W * H, 0);
    float scale = stbtt_ScaleForPixelHeight(&font, (float)fontSize);
    int penX = 1, penY = 1, rowH = 0;

    for (int c = 32; c < 128; c++) {
        int gw, gh, xoff, yoff;
        unsigned char* sdf = stbtt_GetGlyphSDF(
            &font, scale, stbtt_FindGlyphIndex(&font, c),
            4, 128, 32.0f, &gw, &gh, &xoff, &yoff
        );
        if (!sdf) continue;
        if (penX + gw >= W) { penX = 1; penY += rowH + 1; rowH = 0; }
        if (penY + gh >= H) throw std::runtime_error("Font atlas too small");

        for (int y = 0; y < gh; y++)
            for (int x = 0; x < gw; x++)
                atlas[(penY + y) * W + (penX + x)] = sdf[y * gw + x];

        stbtt_FreeSDF(sdf, nullptr);

        SDFGlyph& g = sdfFont.glyphs[c - 32];
        g.u0 = (float)penX / W;
        g.v0 = (float)penY / H;
        g.u1 = (float)(penX + gw) / W;
        g.v1 = (float)(penY + gh) / H;
        g.xoff = (float)xoff;
        g.yoff = (float)yoff;

        int advanceWidth, lsb;
        stbtt_GetCodepointHMetrics(&font, c, &advanceWidth, &lsb);
        g.xadvance = advanceWidth * scale;
        g.width = gw;
        g.height = gh;
        penX += gw + 1;
        rowH = std::max(rowH, gh);
    }

    sdfFont.atlasWidth  = W;
    sdfFont.atlasHeight = H;

    VkDeviceSize imageSize = W * H;

    r->createImage(W, H, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, fontImage, fontImageAllocation);

    uploadTexture(atlas.data(), imageSize, fontImage, VK_FORMAT_R8_UNORM, W, H);

    fontImageView = r->createImageView(fontImage, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    auto info = defaultSamplerInfo();

    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    fontSampler = makeSampler(info);

    std::cout << "[Renderer] SDF Font texture created!\n";
}

void VlkTexSubsys::buildTextMesh(const std::vector<TextDrawCall> &calls) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& call : calls) {
        float x = call.x;
        float y = call.y;

        for (char c : call.text) {
            if (c < 32 || c >= 128) continue;

            const SDFGlyph& g = sdfFont.glyphs[c - 32];

            float gx = x + g.xoff;
            float gy = y + g.yoff;  // yoff is negative = above baseline, pulls text up
            float w = g.width;
            float h = g.height;

            uint32_t base = (uint32_t)vertices.size();

            vertices.push_back({ {gx,     gy},     0.0f, call.color, {g.u0, g.v0} });
            vertices.push_back({ {gx + w, gy},     0.0f, call.color, {g.u1, g.v0} });
            vertices.push_back({ {gx + w, gy + h}, 0.0f, call.color, {g.u1, g.v1} });
            vertices.push_back({ {gx,     gy + h}, 0.0f, call.color, {g.u0, g.v1} });

            indices.insert(indices.end(), {
                base, base + 1, base + 2,
                base + 2, base + 3, base
            });

            x += g.xadvance;
        }
    }

    if (vertices.empty()) {
        r->destroyMesh("__text__");
        return;
    }

    r->updateVertexBuffer("__text__", vertices);
    r->updateIndexBuffer("__text__", indices);
}

void VlkTexSubsys::setTextDrawCalls(const std::vector<TextDrawCall> &calls) {
    textDrawCalls = calls;
}

// =========================================================================
// Sky
// =========================================================================

void VlkTexSubsys::createSkyTexture(const std::string &path) {
    TextureInfo texInfo = loadTextureImage(path);

    VkDeviceSize imageSize = texInfo.width * texInfo.height * 4;

    r->createImage(texInfo.width, texInfo.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, skyImage, skyImageAllocation);

    uploadTexture(
        texInfo.pixels, imageSize,
        skyImage, VK_FORMAT_R8G8B8A8_SRGB,
        texInfo.width, texInfo.height
    );

    skyImageView = r->createImageView(skyImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

    auto info = defaultSamplerInfo();

    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    skySampler = makeSampler(info);

    std::cout << "[Renderer] Sky texture created!\n";
}

void VlkTexSubsys::updateSkyMesh(const CameraParams &cam, float parallaxFactor) {
    float w = static_cast<float>(r->swapChainExtent.width);
    float h = static_cast<float>(r->swapChainExtent.height);

    std::vector<Vertex> verts = {
        {{0, 0}, 0.99f, {1,1,1}, {0, 0}},
        {{w, 0}, 0.99f, {1,1,1}, {1, 0}},
        {{w, h}, 0.99f, {1,1,1}, {1, 1}},
        {{0, h}, 0.99f, {1,1,1}, {0, 1}},
    };
    std::vector<uint32_t> idxs = {0, 1, 2, 2, 3, 0};
    r->updateVertexBuffer("__sky__", verts);
    r->updateIndexBuffer("__sky__", idxs);

    // Store parallax offset for use in drawSky
    skyUVOffset.x = fmod(cam.position.x * parallaxFactor / 1024.0f, 1.0f);
    skyUVOffset.y = 0.0f;
    skyUVScaleX = w / 1024.0f;
    skyUVScaleY = h / 512.0f;
}

// =========================================================================
// Lightmap
// =========================================================================

void VlkTexSubsys::createLightmapTexture(int width, int height) {
    destroyLightmap();

    lightmapTexWidth = width;
    lightmapTexHeight = height;

    r->createImage(
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        lightmapImage, lightmapAllocation
    );

    // Put the image into SHADER_READ_ONLY_OPTIMAL immediately so the descriptor
    // write in createDescriptorSets references a valid layout
    r->transitionImageLayout(
        lightmapImage,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    lightmapImageView = r->createImageView(
        lightmapImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT
    );

    auto info = defaultSamplerInfo();
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    lightmapSampler = makeSampler(info);

    std::cout << "[Renderer] Lightmap texture created (" << width << "x" << height << ")\n";
}

void VlkTexSubsys::updateLightmap(const uint8_t *pixels, int width, int height) {
    if (width != lightmapTexWidth || height != lightmapTexHeight)
        createLightmapTexture(width, height);

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    // Use persistent staging buffer instead of temp one
    r->ensureStagingBuffer(r->stagingOffset + imageSize);
    memcpy(static_cast<char*>(r->stagingMapped) + r->stagingOffset, pixels, imageSize);

    // Record barrier + copy into the open transfer command buffer
    VkImageMemoryBarrier toTransfer{};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = lightmapImage;
    toTransfer.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toTransfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(r->transferCommandBuffer,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    VkBufferImageCopy region{};
    region.bufferOffset = r->stagingOffset;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    vkCmdCopyBufferToImage(r->transferCommandBuffer, r->stagingBuffer, lightmapImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    r->stagingOffset += imageSize;

    VkImageMemoryBarrier toRead{};
    toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.image = lightmapImage;
    toRead.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(r->transferCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toRead);

    // Update descriptors immediately
    for (int i = 0; i < r->MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo info{};
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = lightmapImageView;
        info.sampler = lightmapSampler;

        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = r->descriptorSets[i];
        w.dstBinding = 4;
        w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount = 1;
        w.pImageInfo = &info;

        vkUpdateDescriptorSets(r->device, 1, &w, 0, nullptr);
    }
}

void VlkTexSubsys::destroyLightmap() {
    if (lightmapSampler != VK_NULL_HANDLE)
        vkDestroySampler(r->device, lightmapSampler, nullptr);
    if (lightmapImageView != VK_NULL_HANDLE)
        vkDestroyImageView(r->device, lightmapImageView, nullptr);
    if (lightmapImage != VK_NULL_HANDLE)
        vmaDestroyImage(r->vmaAllocator, lightmapImage, lightmapAllocation);

    lightmapSampler = VK_NULL_HANDLE;
    lightmapImageView = VK_NULL_HANDLE;
    lightmapImage = VK_NULL_HANDLE;
    lightmapTexWidth = 0;
    lightmapTexHeight = 0;
}

// =========================================================================
// Sprite Atlas
// =========================================================================

void VlkTexSubsys::createSpriteAtlas(const std::vector<uint8_t> &pixels, int width, int height) {
    destroySpriteAtlas();

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    r->createImage(
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        spriteImage, spriteAllocation
    );

    uploadTexture(
        pixels.data(), imageSize,
        spriteImage, VK_FORMAT_R8G8B8A8_SRGB,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    );

    spriteImageView = r->createImageView(
        spriteImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    auto info = defaultSamplerInfo();
    info.magFilter = VK_FILTER_NEAREST;
    info.minFilter = VK_FILTER_NEAREST;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    spriteSampler = makeSampler(info);

    if (!r->descriptorSets.empty()) {
        updateSpriteAtlasDescriptors();
    }

    std::cout << "[Renderer] Sprite atlas uploaded: (" << width << "x" << height << ")\n";
}

void VlkTexSubsys::destroySpriteAtlas() {
    if (spriteSampler != VK_NULL_HANDLE)
        vkDestroySampler(r->device, spriteSampler, nullptr);
    if (spriteImageView != VK_NULL_HANDLE)
        vkDestroyImageView(r->device, spriteImageView, nullptr);
    if (spriteImage != VK_NULL_HANDLE)
        vmaDestroyImage(r->vmaAllocator, spriteImage, spriteAllocation);

    spriteSampler = VK_NULL_HANDLE;
    spriteImageView = VK_NULL_HANDLE;
    spriteImage = VK_NULL_HANDLE;
}

void VlkTexSubsys::updateSpriteAtlasDescriptors() {
    for (int i = 0; i < r->MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView   = spriteImageView;
        imgInfo.sampler     = spriteSampler;

        VkWriteDescriptorSet w{};
        w.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet            = r->descriptorSets[i];
        w.dstBinding        = 5;
        w.descriptorType    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount   = 1;
        w.pImageInfo        = &imgInfo;
        vkUpdateDescriptorSets(r->device, 1, &w, 0, nullptr);
    }
}

// =========================================================================
// Sampler creating helpers
// =========================================================================
VkSampler VlkTexSubsys::makeSampler(const VkSamplerCreateInfo &info) {
    VkSampler sampler;

    if (vkCreateSampler(r->device, &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sampler!");

    return sampler;
}

VkSamplerCreateInfo VlkTexSubsys::defaultSamplerInfo() {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;

    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 1.0f;

    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;

    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;

    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    return info;
}

// =========================================================================
// Texture creation helper
// =========================================================================

TextureInfo VlkTexSubsys::loadTextureImage(const std::string &path) {
    int w, h, ch;

    stbi_uc *pixels = stbi_load(
        path.c_str(),
        &w, &h,
        &ch,
        STBI_rgb_alpha
    );

    if (!pixels)
        throw std::runtime_error("Failed to load sky texture: " + path);

    if (w % 8 != 0 || h % 8 != 0)
        throw std::runtime_error("Texture atlas dimensions must be multiplies of 8!");

    TextureInfo info{};
    info.width = w;
    info.height = h;
    info.channels = ch;
    info.pixels = pixels;

    return info;
}

void VlkTexSubsys::uploadTexture(const void* pixels, VkDeviceSize imageSize, VkImage image, VkFormat format, uint32_t width, uint32_t height) {
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    r->createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        stagingBuffer,
        stagingAllocation
    );

    void* data;
    vmaMapMemory(r->vmaAllocator, stagingAllocation, &data);
    memcpy(data, pixels, imageSize);
    vmaUnmapMemory(r->vmaAllocator, stagingAllocation);

    r->transitionImageLayout(
        image, format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    r->copyBufferToImage(stagingBuffer, image, width, height);

    r->transitionImageLayout(
        image, format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    vmaDestroyBuffer(r->vmaAllocator, stagingBuffer, stagingAllocation);
}