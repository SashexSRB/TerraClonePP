#include "VlkMeshSubsys.h"
#include "VlkRenderer.h"

#include <cstring>

// Public section

void VlkMeshSubsys::updateVertexBuffer(const std::string &name, const std::vector<Vertex> &vertices) {
    auto& mesh = meshes[name];

    uploadToGpuBuffer(
        mesh.vertexBuffer,
        mesh.vertexAllocation,
        vertices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

}

void VlkMeshSubsys::updateVertexBuffer(int64_t key, const std::vector<Vertex> &vertices) {
    auto& mesh = chunkMeshes[key];

    uploadToGpuBuffer(
        mesh.vertexBuffer,
        mesh.vertexAllocation,
        vertices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );
}

void VlkMeshSubsys::updateIndexBuffer(const std::string &name, const std::vector<uint32_t> &indices) {
    auto& mesh = meshes[name];

    uploadToGpuBuffer(
        mesh.indexBuffer,
        mesh.indexAllocation,
        indices,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    mesh.indexCount = indices.size();
}

void VlkMeshSubsys::updateIndexBuffer(int64_t key, const std::vector<uint32_t> &indices) {
    auto& mesh = chunkMeshes[key];

    uploadToGpuBuffer(
        mesh.indexBuffer,
        mesh.indexAllocation,
        indices,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );

    mesh.indexCount = indices.size();
}

void VlkMeshSubsys::destroyMesh(const std::string &name) {
    destroyMeshImpl(meshes, name);
}

void VlkMeshSubsys::destroyMesh(int64_t key) {
    destroyMeshImpl(chunkMeshes, key);
}

void VlkMeshSubsys::setChunkKeys(const std::vector<int64_t>& keys) {
    chunkKeys = keys;
}

// Private section

template<typename Key>
MeshBuffer& VlkMeshSubsys::getMeshBuffer(std::unordered_map<Key, MeshBuffer>& map, const Key& key) {
    return map[key];
}

template<typename T>
void VlkMeshSubsys::uploadToGpuBuffer(VkBuffer& buffer, VmaAllocation& allocation, const std::vector<T>& data, VkBufferUsageFlags useFlags) {
    VkDeviceSize bufferSize = sizeof(T) * data.size();
    if (buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(r->vmaAllocator, buffer, allocation);

    r->ensureStagingBuffer(r->stagingOffset + bufferSize);
    memcpy(static_cast<char*>(r->stagingMapped) + r->stagingOffset, data.data(), bufferSize);

    r->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | useFlags,
        VMA_MEMORY_USAGE_GPU_ONLY,
        buffer, allocation
    );

    r->copyBuffer(r->stagingBuffer, buffer, bufferSize, r->stagingOffset);
    r->stagingOffset += bufferSize;
}

template<typename Key>
void VlkMeshSubsys::destroyMeshImpl(std::unordered_map<Key, MeshBuffer>& map, const Key& key) {
    auto it = map.find(key);
    if (it == map.end()) return;

    MeshBuffer& mesh = it->second;
    if (mesh.vertexBuffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(r->vmaAllocator, mesh.vertexBuffer, mesh.vertexAllocation);
    if (mesh.indexBuffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(r->vmaAllocator, mesh.indexBuffer, mesh.indexAllocation);
    map.erase(it);
}