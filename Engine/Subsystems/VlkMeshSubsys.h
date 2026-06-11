#pragma once
#include "VlkTypes.h"
#include <string>
#include <vector>

class VlkRenderer;

class VlkMeshSubsys {
public:
    VlkRenderer* r = nullptr;

    std::unordered_map<std::string, MeshBuffer> meshes;       // player, inventory, sky, text
    std::unordered_map<int64_t, MeshBuffer> chunkMeshes;      // chunks only

    std::vector<int64_t> chunkKeys;

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
     * Updates vertex buffer for a mesh based on a numerical key (Chunks)
     *
     * Replaces GPU data for an existing chunk
     *
     * @param key Mesh identifier
     * @param vertices New vertex data
     */
    void updateVertexBuffer(
        int64_t key,
        const std::vector<Vertex> &vertices
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
     * Updates index buffer for a mesh based on a numerical key (Chunks)
     *
     * @param key Mesh identifier
     * @param indices New index data
     */
    void updateIndexBuffer(
        int64_t key,
        const std::vector<uint32_t> &indices
    );

    /**
     * Deletes a mesh and frees GPU memory.
     *
     * @param name Mesh identifier
     */
    void destroyMesh(
        const std::string& name
    );

    /**
     * Deletes a mesh and frees GPU memory.
     *
     * @param key Mesh identifier
     */
    void destroyMesh(
        int64_t key
    );

    /**
     * Sets active chunk mesh keys for world rendering.
     *
     * @param keys Vector of numeric keys to set.
     */
    void setChunkKeys(const std::vector<int64_t>& keys);

private:

    /**
     * Retrieves the mesh buffer associated with a key.
     *
     * @tparam Key Key type used as the key in the map. Must satisfy the requirements for std::unordered_map keys.
     * @param map Map containing mesh buffers.
     * @param key Key identifying teh mesh buffer.
     * @return Reference to the mesh buffer associated with the key.
     */
    template<typename Key>
    MeshBuffer& getMeshBuffer(
        std::unordered_map<Key, MeshBuffer>& map,
        const Key& key
    );

    /**
     * Uploads mesh buffer to the GPU for drawing.
     *
     * @tparam T Buffer type. Should be a std::vector of Vertex or uint32_t.
     * @param buffer MeshBuffer passed to the GPU.
     * @param allocation Reference to the VMA allocation for the buffer.
     * @param data Data being uploaded to the GPU.
     * @param useFlags Vulkan usage flags for the buffer
     */
    template<typename T>
    void uploadToGpuBuffer(
        VkBuffer& buffer,
        VmaAllocation& allocation,
        const std::vector<T>& data,
        VkBufferUsageFlags useFlags
    );

    /**
     * Templated implementation for destroyMesh overloads.
     *
     * @tparam Key Type of the mesh being destroyed. (numerically keyed / string-keyed)
     * @param map Map containing the mesh buffers to destroy.
     * @param key Key identifying the mesh buffer
     */
    template<typename Key>
    void destroyMeshImpl(
        std::unordered_map<Key, MeshBuffer>& map,
        const Key& key
    );
};