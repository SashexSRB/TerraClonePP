#include "VlkFrameSubsys.h"
#include "VlkRenderer.h"
#include <stdexcept>
#include <iostream>

void VlkFrameSubsys::createCommandPool() {
    QueueFamilyIndices indices = r->findQueueFamilies(r->physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(r->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");

    std::cout << "[FrameSubsys] Command pool created.\n";
}

void VlkFrameSubsys::createCommandBuffers() {
    commandBuffers.resize(r->MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(r->device, &ai, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");

    std::cout << "[FrameSubsys] Command buffers created.\n";
}

void VlkFrameSubsys::createSyncObjects() {
    imageAvailableSemaphores.resize(r->MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(r->MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(r->MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < r->MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(r->device, &si, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(r->device, &si, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(r->device, &fi, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects!");
    }

    std::cout << "[FrameSubsys] Sync objects created.\n";
}

void VlkFrameSubsys::createUniformBuffers() {
    VkDeviceSize size = sizeof(UniformBufferObject);
    uniformBuffers.resize(r->MAX_FRAMES_IN_FLIGHT);
    uniformAllocations.resize(r->MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(r->MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < r->MAX_FRAMES_IN_FLIGHT; ++i) {
        r->createBuffer(
            size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            uniformBuffers[i], uniformAllocations[i]
        );

        vmaMapMemory(r->vmaAllocator, uniformAllocations[i], &uniformBuffersMapped[i]);
    }

    std::cout << "[FrameSubsys] Uniform buffers created.\n";
}

void VlkFrameSubsys::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount = static_cast<uint32_t>(r->MAX_FRAMES_IN_FLIGHT);
    sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[1].descriptorCount = static_cast<uint32_t>(r->MAX_FRAMES_IN_FLIGHT * 5);

    VkDescriptorPoolCreateInfo pi{};
    pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pi.pPoolSizes = sizes.data();
    pi.maxSets = static_cast<uint32_t>(r->MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(r->device, &pi, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");

    std::cout << "[FrameSubsys] Descriptor pool created.\n";
}

void VlkFrameSubsys::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(r->MAX_FRAMES_IN_FLIGHT, r->descriptorSetLayout);

    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = descriptorPool;
    ai.descriptorSetCount = static_cast<uint32_t>(r->MAX_FRAMES_IN_FLIGHT);
    ai.pSetLayouts = layouts.data();

    descriptorSets.resize(r->MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(r->device, &ai, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    auto& tex = r->texSubsys;

    for (int i = 0; i < r->MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo uboInfo{};
        uboInfo.buffer = uniformBuffers[i];
        uboInfo.range = sizeof(UniformBufferObject);

        auto imgInfo = [](VkImageView v, VkSampler s) {
            VkDescriptorImageInfo ii{};
            ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ii.imageView = v;
            ii.sampler = s;
            return ii;
        };

        auto tileInfo   = imgInfo(r->textureImageView,   r->textureSampler);
        auto fontInfo   = imgInfo(tex.fontImageView,     tex.fontSampler);
        auto skyInfo    = imgInfo(tex.skyImageView,      tex.skySampler);
        auto lightInfo  = imgInfo(tex.lightmapImageView, tex.lightmapSampler);
        auto spriteInfo = imgInfo(tex.spriteImageView,   tex.spriteSampler);

        auto makeWrite = [&](uint32_t binding, VkDescriptorType type) {
            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSets[i];
            w.dstBinding = binding;
            w.descriptorType = type;
            w.descriptorCount = 1;
            return w;
        };

        auto w0 = makeWrite(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         w0.pBufferInfo = &uboInfo;
        auto w1 = makeWrite(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w1.pImageInfo  = &tileInfo;
        auto w2 = makeWrite(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w2.pImageInfo  = &fontInfo;
        auto w3 = makeWrite(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w3.pImageInfo  = &skyInfo;
        auto w4 = makeWrite(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w4.pImageInfo  = &lightInfo;
        auto w5 = makeWrite(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); w5.pImageInfo  = &spriteInfo;

        std::array<VkWriteDescriptorSet, 6> writes = {w0, w1, w2, w3, w4, w5};

        vkUpdateDescriptorSets(
            r->device,
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }

    std::cout << "[FrameSubsys] Descriptor sets created.\n";
}

void VlkFrameSubsys::updateUniformBuffer(uint32_t currentImage, const CameraParams& cam) {
    // Write directly into persistently mapped memory, no staging, no fence
    UniformBufferObject* ubo = static_cast<UniformBufferObject*>(uniformBuffersMapped[currentImage]);

    ubo->proj       = glm::mat4(1.0f);
    ubo->proj[0][0] = 2.0f / cam.visibleWidth;
    ubo->proj[1][1] = 2.0f / cam.visibleHeight;
    ubo->proj[3][0] = -1.0f;
    ubo->proj[3][1] = -1.0f;

    ubo->view = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(
            -(cam.position.x - cam.visibleWidth / 2.0f),
            -(cam.position.y - cam.visibleHeight / 2.0f),
            0.0f
        )
    );

    ubo->model = glm::mat4(1.0f);
}

void VlkFrameSubsys::bindPipeline(VkCommandBuffer cmd, VkPipeline pipeline, const VkViewport &viewport, const VkRect2D &scissor) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        r->pipelineLayout,
        0,
        1,
        &descriptorSets[r->currentFrame],
        0,
        nullptr
    );
}

void VlkFrameSubsys::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = 0;
    bi.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(commandBuffer, &bi) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffer!");

    VkRenderPassBeginInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpi.renderPass = r->renderPass;
    rpi.framebuffer = r->swapChainFramebuffers[imageIndex];
    rpi.renderArea.offset = { 0, 0 };
    rpi.renderArea.extent = r->swapChainExtent;

    std::array<VkClearValue, 2> clear{};
    clear[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear[1].depthStencil = {1.0f, 0};

    rpi.clearValueCount = static_cast<uint32_t>(clear.size());
    rpi.pClearValues = clear.data();

    vkCmdBeginRenderPass(commandBuffer, &rpi, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp{};
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width = static_cast<float>(r->swapChainExtent.width);
    vp.height = static_cast<float>(r->swapChainExtent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = r->swapChainExtent;

    // Sky
    bindPipeline(commandBuffer, r->uiPipeline, vp, scissor);
    r->drawSky(commandBuffer);

    // World chunks
    bindPipeline(commandBuffer, r->graphicsPipeline, vp, scissor);
    {
        UIPushConstants wp{};
        wp.useLighting = 1;
        wp.lightmapOrigin = r->texSubsys.lastLightmapOrigin;
        wp.lightmapSize = r->texSubsys.lastLightmapSize;

        vkCmdPushConstants(
            commandBuffer,
            r->pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(UIPushConstants),
            &wp
        );

        for (const auto& key : r->meshSubsys.chunkKeys)
            r->draw(key, commandBuffer);
    }

    // Player
    {
        UIPushConstants pp{};
        pp.useSprite = 1;
        pp.useLighting = 1;
        pp.lightmapOrigin = r->texSubsys.lastLightmapOrigin;
        pp.lightmapSize = r->texSubsys.lastLightmapSize;

        vkCmdPushConstants(
            commandBuffer,
            r->pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(UIPushConstants),
            &pp
        );

        r->draw("player", commandBuffer);
    }

    bindPipeline(commandBuffer, r->uiPipeline, vp, scissor);
    r->drawUI("inventory", commandBuffer);
    r->drawSprite("inventory_sprites", commandBuffer);
    r->drawText(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void VlkFrameSubsys::drawFrame(GLFWwindow *window, bool &framebufferResized, const CameraParams& cam) {
    vkWaitForFences(
        r->device,
        1,
        &inFlightFences[r->currentFrame],
        VK_TRUE,
        UINT64_MAX
    );

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        r->device,
        r->swapChain,
        UINT64_MAX,
        imageAvailableSemaphores[r->currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        r->recreateSwapChain(window);
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    updateUniformBuffer(r->currentFrame, cam);
    r->lastCam = cam;

    vkResetFences(r->device, 1, &inFlightFences[r->currentFrame]);
    vkResetCommandBuffer(commandBuffers[r->currentFrame], 0);
    recordCommandBuffer(commandBuffers[r->currentFrame], imageIndex);

    VkSemaphore waitSems[] = {imageAvailableSemaphores[r->currentFrame]};
    VkPipelineStageFlags wst[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitSems;
    si.pWaitDstStageMask = wst;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &commandBuffers[r->currentFrame];

    VkSemaphore signalSems[] = {renderFinishedSemaphores[r->currentFrame]};
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = signalSems;

    if (vkQueueSubmit(r->graphicsQueue, 1, &si, inFlightFences[r->currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = signalSems;

    VkSwapchainKHR swapChains[] = {r->swapChain};
    pi.swapchainCount = 1;
    pi.pSwapchains = swapChains;
    pi.pImageIndices = &imageIndex;

    pi.pResults = nullptr;

    result = vkQueuePresentKHR(r->presentQueue, &pi);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        r->recreateSwapChain(window);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    r->currentFrame = (r->currentFrame + 1) % r->MAX_FRAMES_IN_FLIGHT;
}
