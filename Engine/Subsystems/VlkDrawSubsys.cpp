#include "VlkDrawSubsys.h"

#include "VlkRenderer.h"

void VlkDrawSubsys::draw(const std::string &name, VkCommandBuffer commandBuffer) {
    auto it = r->meshSubsys.meshes.find(name);
    if (it != r->meshSubsys.meshes.end()) drawMesh(it->second, commandBuffer);
}

// Overload
void VlkDrawSubsys::draw(int64_t key, VkCommandBuffer commandBuffer) {
    auto it = r->meshSubsys.chunkMeshes.find(key);
    if (it != r->meshSubsys.chunkMeshes.end()) drawMesh(it->second, commandBuffer);
}

void VlkDrawSubsys::drawUI(const std::string &name, VkCommandBuffer commandBuffer) {
    drawWithPush(name, commandBuffer, makeOrthoPush());
}

void VlkDrawSubsys::drawText(VkCommandBuffer commandBuffer) {
    auto push = makeOrthoPush();
    push.useFont = 1;
    drawWithPush("__text__", commandBuffer, push);
}

void VlkDrawSubsys::drawSky(VkCommandBuffer commandBuffer) {
    auto push = makeOrthoPush();
    push.useSky = 1;
    push.skyUVOffset = r->texSubsys.skyUVOffset;
    push.skyUVScale = {r->texSubsys.skyUVScaleX, r->texSubsys.skyUVScaleY};
    drawWithPush("__sky__", commandBuffer, push);
}

void VlkDrawSubsys::drawSprite(const std::string &name, VkCommandBuffer commandBuffer) {
    auto push = makeOrthoPush();
    push.useSprite = 1;
    drawWithPush(name, commandBuffer, push);
}

UIPushConstants VlkDrawSubsys::makeOrthoPush() {
    UIPushConstants push{};
    push.useUIProj = 1;
    push.proj = glm::mat4(1.0f);
    push.proj[0][0] = 2.0f / r->swapChainExtent.width;
    push.proj[1][1] = 2.0f / r->swapChainExtent.height;
    push.proj[3][0] = -1.0f;
    push.proj[3][1] = -1.0f;
    return push;
}

void VlkDrawSubsys::drawMesh(const MeshBuffer &mesh, VkCommandBuffer commandBuffer) {
    if (mesh.indexCount == 0) return;
    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
}

void VlkDrawSubsys::drawWithPush(const std::string &name, VkCommandBuffer commandBuffer, const UIPushConstants &push) {
    auto it = r->meshSubsys.meshes.find(name);
    if (it == r->meshSubsys.meshes.end() || it->second.indexCount == 0) return;

    vkCmdPushConstants(
        commandBuffer,
        r->pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(UIPushConstants),
        &push
    );

    drawMesh(it->second, commandBuffer);

    const UIPushConstants reset{};
    vkCmdPushConstants(
        commandBuffer,
        r->pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(UIPushConstants),
        &reset
    );
}