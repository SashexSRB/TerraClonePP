#include "Game.h"
#include "Constants.h"
#include "Entity/Physics.h"
#include "Items/Item.h"
#include "VulkanApp.h"
#include "World/ChunkMesher.h"
#include "Engine/VlkTypes.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <glm/gtc/matrix_transform.hpp>

VulkanApp app;

// =========================================================================
// Construction / Lifetime
// =========================================================================

Game::Game(GLFWwindow *window, VlkRenderer &renderer)
    : renderer(renderer), window(window),
      world(8400, 2400), serializer(SAVE_PATH),
      lastAutoSave(std::chrono::steady_clock::now()) {

    loadOrGenerateWorld();

    int spawnX = world.getWidth() / 2;
    int spawnTileY = 0;
    for (int y = 0; y < world.getHeight(); ++y) {
        if (world.getTile(spawnX, y).isActive) {
            spawnTileY = y;
            break;
        }
    }

    player.position = {
        spawnX * Constants::TileSize,
        (spawnTileY - 4) * Constants::TileSize
    };

    CameraParams cam = computeCameraParams(
            player, world,
            renderer.swapChainExtent.width,
            renderer.swapChainExtent.height,
            Constants::TileSize, Constants::VisibleTilesX
    );

    for (auto& [id, item] : Registry::items) {
        if (!item.spritePath.empty()) spriteAtlas.add(id, item.spritePath);
    }
    spriteAtlas.add(SPRITE_PLAYER, ASSET_PATH "Assets/Sprites/player.png");
    int aw, ah;
    spriteAtlas.build(aw, ah);

    updateBuffers(cam);

    std::cout << "[Game] Starting physics thread...\n";
    physicsThread = std::thread(&Game::gameLoopThread, this);
    std::cout << "[Game] Starting mesh thread...\n";
    meshThread = std::thread(&Game::meshWorkerThread, this);
    std::cout << "[Game] Starting lighting thread...\n";
    lightmapThread = std::thread(&Game::lightmapWorkerThread, this);
}

Game::~Game() {
    running = false;
    if (physicsThread.joinable())
        physicsThread.join();
    std::cout << "[Game] Physics thread stopped.\n";

    meshThreadRunning = false;
    meshCV.notify_all();
    if (meshThread.joinable())
        meshThread.join();
    std::cout << "[Game] Mesh thread stopped.\n";

    lightmapThreadRunning = false;
    lightmapCV.notify_all();
    if (lightmapThread.joinable())
        lightmapThread.join();
    std::cout << "[Game] Lighting thread stopped.\n";
}

// =========================================================================
// Public API
// =========================================================================

void Game::run() {
    double lastTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        // FPS counter
        frameCount++;

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;

        if (delta >= 1.0) {
            double fps = frameCount / delta;

            std::string title =
                "TerraClone - FPS: " + std::to_string(static_cast<int>(fps));

            glfwSetWindowTitle(window, title.c_str());

            frameCount = 0;
            lastTime = currentTime;
        }

        // Autosave every 5 minutes
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::minutes>(now - lastAutoSave).count() >= 5) {
            saveWorld();
            lastAutoSave = now;
        }

        CameraParams cam;
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            handleInput();
            cam = computeCameraParams(
                player, world,
                renderer.swapChainExtent.width,
                renderer.swapChainExtent.height,
                Constants::TileSize, Constants::VisibleTilesX
            );
        }

        updateBuffers(cam);

        {
            std::lock_guard<std::mutex> lock(renderMutex);
            renderer.drawFrame(window, app.framebufferResized, cam);
        }

        glfwPollEvents();
    }

    if (physicsThread.joinable()) {
        physicsThread.join();
    }
}

void Game::onScroll(double yoffset) {
    if (yoffset > 0)
        player.inventory.activeSlot = (player.inventory.activeSlot - 1 + INVENTORY_SLOTS) % INVENTORY_SLOTS;
    else if (yoffset < 0)
        player.inventory.activeSlot = (player.inventory.activeSlot + 1) % INVENTORY_SLOTS;

    inventoryMeshDirty = true;
}

// #########################################################################
// PRIVATE SECTION
// #########################################################################

// =========================================================================
// Serialization
// =========================================================================

void Game::saveWorld() {
    serializer.save(world, player.inventory);
}

void Game::loadOrGenerateWorld() {
    if (!serializer.load(world, player.inventory)) {
        std::cout << "[Game] No save found, generating new world...\n";
        unsigned int seed = static_cast<unsigned int>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        world.generate(seed);

        player.inventory.addItem(1002, 1); // Copper Sword
        player.inventory.addItem(1000, 1); // Copper Pickaxe
        player.inventory.addItem(1001, 1); // Copper Axe
    }
}

// =========================================================================
// Camera Helpers
// =========================================================================

CameraParams Game::computeCameraParams(
    const Player &player,
    const World &world,
    int windowWidth,
    int windowHeight,
    float tileSize,
    float visibleTilesX
) {
    CameraParams cam;

    cam.visibleWidth = visibleTilesX * tileSize;
    cam.visibleHeight = cam.visibleWidth * windowHeight / windowWidth;

    cam.position = player.position;

    // Clamp camera to world bounds
    float worldWidth = world.getWidth() * tileSize;
    float worldHeight = world.getHeight() * tileSize;

    cam.position.x = std::max(
        cam.visibleWidth / 2.0f,
        std::min(cam.position.x, worldWidth - cam.visibleWidth / 2.0f)
    );

    cam.position.y = std::max(
        cam.visibleHeight / 2.0f,
        std::min(cam.position.y, worldHeight - cam.visibleHeight / 2.0f)
    );

    return cam;
}

glm::ivec2 Game::screenToTile(
    double mouseX,
    double mouseY,
    const CameraParams &cam,
    int windowWidth,
    int windowHeight,
    float tileSize
) {
    // Convert screen pixels to NDC (-1..1)
    float ndcX = static_cast<float>(mouseX) / windowWidth * 2.0f - 1.0f;
    float ndcY = static_cast<float>(mouseY) / windowHeight * 2.0f - 1.0f;

    // Map NDC to world coordinates
    float worldX = cam.position.x - cam.visibleWidth  / 2.0f + (ndcX + 1.0f) / 2.0f * cam.visibleWidth;
    float worldY = cam.position.y - cam.visibleHeight / 2.0f + (ndcY + 1.0f) / 2.0f * cam.visibleHeight;

    // World -> tile indices
    int tileX = static_cast<int>(worldX / tileSize);
    int tileY = static_cast<int>(worldY / tileSize);

    // Clamp to world
    tileX = std::clamp(tileX, 0, world.getWidth() - 1);
    tileY = std::clamp(tileY, 0, world.getHeight() - 1);

    return glm::ivec2(tileX, tileY);
}

// =========================================================================
// Game Loop
// =========================================================================

void Game::update(float deltaTime) {
    glm::vec2 prevPos = player.position;
    applyMovement(player, inputState, deltaTime);
    resolveCollisions(player, world, deltaTime);
    if (player.position != prevPos) {
        playerMeshDirty = true;
        skyMeshDirty = true;
        lightmapDirty = true;
    }
}

void Game::handleInput() {
    // Graceful shutdown on ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        std::cout << "[Game] Shutting down...\n";
        saveWorld();
        running = false;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    // Save world on F5
    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
        saveWorld();
    }

    inputState.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    inputState.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    inputState.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    CameraParams cam = computeCameraParams(
        player, world,
        renderer.swapChainExtent.width,
        renderer.swapChainExtent.height,
        Constants::TileSize, Constants::VisibleTilesX
    );

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    float moveSpeed = player.moveSpeed;
    player.velocity.x = 0.0f;

    glm::ivec2 tileCoords = screenToTile(
        xpos, ypos, cam,
        renderer.swapChainExtent.width,
        renderer.swapChainExtent.height,
        Constants::TileSize
    );

    glm::vec2 playerCenter = {
        player.position.x + Constants::PlayerWidth / 2.0f,
        player.position.y + Constants::PlayerHeight / 2.0f
    };

    glm::vec2 tileCenter = {
        tileCoords.x * Constants::TileSize + Constants::TileSize / 2.0f,
        tileCoords.y * Constants::TileSize + Constants::TileSize / 2.0f
    };

    float distance = glm::length(tileCenter - playerCenter) / Constants::TileSize;
    bool inReach = distance <= Constants::BlockReach;

    // Slot selection 1-9 (0 for slot 10)
    for (int i = 0; i < 9; ++i) {
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS) {
            player.inventory.activeSlot = i;
            inventoryMeshDirty = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        player.inventory.activeSlot = 9;
        inventoryMeshDirty = true;
    }

    // Left click: mine or place
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && inReach) {
        if (tileCoords.x >= 0 && tileCoords.x < world.getWidth() &&
            tileCoords.y >= 0 && tileCoords.y < world.getHeight()) {

            Tile t = world.getTile(tileCoords.x, tileCoords.y);
            int      activeSlot = player.inventory.activeSlot.load();
            uint32_t itemId     = player.inventory.slots[activeSlot].itemId;
            const GameItem *item = Registry::isValid(itemId)
                ? &Registry::get(itemId) : nullptr;

            if (t.isActive) {
                const GameItem &tileItem = Registry::get(t.tileId);

                bool correctTool = false;
                bool enoughPower = true;

                if (item && item->itemType == ItemType::Tool) {
                    switch (tileItem.breakType) {
                        case TileBreakType::Pickaxe:
                            correctTool = (item->toolType == ToolType::Pickaxe); break;
                        case TileBreakType::Axe:
                            correctTool = (item->toolType == ToolType::Axe);     break;
                        case TileBreakType::Hammer:
                            correctTool = (item->toolType == ToolType::Hammer);  break;
                        case TileBreakType::None:
                            correctTool = false; break;
                    }
                    enoughPower = (item->toolPower >= tileItem.requiredPower);
                }

                if (correctTool && enoughPower) {
                    if (tileCoords != miningTile) {
                        mineTimer  = 0.0f;
                        miningTile = tileCoords;
                    }

                    static auto lastMine = std::chrono::high_resolution_clock::now();
                    auto now = std::chrono::high_resolution_clock::now();
                    float dt = std::chrono::duration<float>(now - lastMine).count();
                    lastMine = now;

                    mineTimer += dt;
                    if (mineTimer >= tileItem.breakTime / item->toolSpeed) {
                        player.inventory.addItem(t.tileId, 1);
                        t.isActive = false;
                        world.setTile(tileCoords.x, tileCoords.y, t);
                        mineTimer  = 0.0f;
                        miningTile = {-1, -1};
                        inventoryMeshDirty = true;
                        lightmapDirty = true;
                    }
                } else {
                    mineTimer  = 0.0f;
                    miningTile = {-1, -1};
                }
            } else {
                // Placing
                if (item && item->itemType == ItemType::Tile && item->id != 0) {
                    t.tileId   = static_cast<uint16_t>(item->id);
                    t.isActive = true;
                    world.setTile(tileCoords.x, tileCoords.y, t);
                    player.inventory.removeItem(activeSlot);
                    inventoryMeshDirty = true;
                    lightmapDirty = true;
                }
                mineTimer  = 0.0f;
                miningTile = {-1, -1};
            }
        }
    } else {
        mineTimer  = 0.0f;
        miningTile = {-1, -1};
    }
}

void Game::updateBuffers(const CameraParams &cam) {
    int playerTileX = static_cast<int>(player.position.x / 32.0f);
    int playerTileY = static_cast<int>(player.position.y / 32.0f);

    bool chunksChanged = world.chunks.update(playerTileX, playerTileY, 2);

    {
        std::lock_guard<std::mutex> lock(renderMutex);

        {
            int camTileX = static_cast<int>(cam.position.x / Constants::TileSize);
            int camTileY = static_cast<int>(cam.position.y / Constants::TileSize);
            glm::ivec2 camTile = {camTileX, camTileY};

            int tileDist = std::abs(camTile.x - lastLightmapCamTile.x) +
                           std::abs(camTile.y - lastLightmapCamTile.y);

            if (lightmapDirty || tileDist >= 2) {
                int visX = static_cast<int>(cam.visibleWidth  / Constants::TileSize) + 2;
                int visY = static_cast<int>(cam.visibleHeight / Constants::TileSize) + 2;

                lightmapRequest = {camTileX, camTileY, visX, visY};
                lightmapPending = true;
                lightmapCV.notify_one();

                lastLightmapCamTile = camTile;
                lightmapDirty = false;
            }
        }

        renderer.beginTransferBatch();

        // Clean up GPU buffers for chunks that are no longer loaded
        if (chunksChanged) {
            std::unordered_set<int64_t> validKeys;
            for (auto &kv : world.chunks.getChunks()) {
                validKeys.insert(kv.first);
            }

            std::vector<int64_t> toRemove;
            for (auto &kv : renderer.meshSubsys.chunkMeshes) {
                if (!validKeys.count(kv.first)) toRemove.push_back(kv.first);
            }

            for (auto &key : toRemove) {
                renderer.destroyMesh(key);
            }
        }

        // Queue dirty chunks for off-thread meshing
        {
            std::lock_guard<std::mutex> qlock(meshQueueMutex);
            for (auto &kv : world.chunks.getChunks()) {
                Chunk &chunk = kv.second;
                if (!chunk.needsUpdate) continue;
                meshQueue.push_back(chunk); // copy for thread safety
                chunk.needsUpdate = false;
            }
            if (!meshQueue.empty())
                meshCV.notify_one();
        }

        // Upload completed meshes to GPU
        {
            std::vector<MeshResult> ready;
            {
                std::lock_guard<std::mutex> rlock(meshResultMutex);
                ready = std::move(meshResults);
                meshResults.clear();
            }

            for (auto &result : ready) {
                if (result.vertices.empty() || result.indices.empty()) {
                    renderer.destroyMesh(result.chunkKey);
                    continue;
                }
                renderer.updateVertexBuffer(result.chunkKey, result.vertices);
                renderer.updateIndexBuffer(result.chunkKey, result.indices);
            }
        }

        // Rebuild visible chunk keys every frame for frustum culling
        cachedChunkKeys.clear();
        cachedChunkKeys.reserve(world.chunks.getChunks().size());
        for (auto &kv : world.chunks.getChunks()) {
            const Chunk &chunk = kv.second;
            if (isChunkVisible(chunk.chunkX, chunk.chunkY, cam))
                cachedChunkKeys.push_back(kv.first);
        }
        renderer.setChunkKeys(cachedChunkKeys);

        // Update sky mesh when dirty
        if (skyMeshDirty) {
            renderer.updateSkyMesh(cam, 0.15f);
            skyMeshDirty = false;
        }

        // Player mesh
        if (playerMeshDirty) {
            generatePlayerVertices(player, playerVertices, playerIndices, spriteAtlas);
            renderer.updateVertexBuffer("player", playerVertices);
            renderer.updateIndexBuffer("player", playerIndices);
            playerMeshDirty = false;
        }

        // Inventory and text mesh
        if (inventoryMeshDirty) {
            generateInventoryVertices(player.inventory, inventoryVertices, inventoryIndices, inventorySpriteVerts, inventorySpriteIndices, spriteAtlas);
            if (!inventoryVertices.empty())
                renderer.updateVertexBuffer("inventory", inventoryVertices);
            if (!inventoryIndices.empty())
                renderer.updateIndexBuffer("inventory", inventoryIndices);
            if (!inventorySpriteVerts.empty()) {
                renderer.updateVertexBuffer("inventory_sprites", inventorySpriteVerts);
                renderer.updateIndexBuffer("inventory_sprites", inventorySpriteIndices);
            } else {
                renderer.destroyMesh("inventory_sprites");
            }

            std::vector<TextDrawCall> textCalls;
            for (int i = 0; i < INVENTORY_SLOTS; ++i) {
                if (player.inventory.slots[i].itemId == 1000 ||
                    player.inventory.slots[i].itemId == 1001 ||
                    player.inventory.slots[i].itemId == 1002) continue;

                if (!player.inventory.slots[i].empty()) {
                    float x = Constants::InventoryPadding + i * (Constants::InventorySlotSize + Constants::InventoryPadding);
                    float y = Constants::InventoryPadding;
                    std::string countStr = std::to_string(player.inventory.slots[i].count);
                    textCalls.push_back({
                        countStr,
                        x + Constants::InventorySlotSize - 4.0f * countStr.size(),
                        y + Constants::InventorySlotSize - 4.0f,
                        {1.0f, 1.0f, 1.0f}
                    });
                }
            }
            renderer.setTextDrawCalls(textCalls);
            renderer.buildTextMesh(textCalls);
            inventoryMeshDirty = false;
        }

        // Lightmap upload — now part of the batch, no GPU stall
        if (lightmapReady.exchange(false)) {
            renderer.updateLightmap(
                lightmapBack.pixels.data(),
                lightmapBack.width,
                lightmapBack.height
            );
            renderer.lastLightmapOrigin = {
                static_cast<float>(lightmapBack.originX),
                static_cast<float>(lightmapBack.originY)
            };
            renderer.lastLightmapSize = {
                static_cast<float>(lightmapBack.width),
                static_cast<float>(lightmapBack.height)
            };
        }

        renderer.endTransferBatch();
    }
    renderer.updateUniformBuffer(renderer.currentFrame, cam);
}

void Game::gameLoopThread() {
    std::cout << "[Game] Thread started.\n";
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // lock the world while updating
        {
            std::lock_guard<std::mutex> lock(worldMutex);
            update(deltaTime);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::cout << "[Game] Thread exiting.\n";
}

// =========================================================================
// Mesh Generation Thread
// =========================================================================

void Game::meshWorkerThread() {
    std::vector<QuadSpec> scratch;

    while (meshThreadRunning) {
        std::vector<Chunk> batch;

        {
            std::unique_lock<std::mutex> lock(meshQueueMutex);
            meshCV.wait(lock, [this] {
                return !meshQueue.empty() || !meshThreadRunning;
            });
            batch = std::move(meshQueue);
            meshQueue.clear();
        }

        for (auto& chunk : batch) {
            MeshResult result;
            result.chunkKey = ChunkManager::chunkKey(chunk.chunkX, chunk.chunkY);
            ChunkMesher::mesh(chunk, result.vertices, result.indices, world.chunks.getChunkSize(), scratch);

            std::lock_guard<std::mutex> lock(meshResultMutex);
            meshResults.push_back(std::move(result));
        }
    }
}

// =========================================================================
// Lightmap Thread
// =========================================================================

void Game::lightmapWorkerThread() {
    LightMap lm;
    while (lightmapThreadRunning) {
        {
            std::unique_lock<std::mutex> lock(lightmapMutex);
            lightmapCV.wait(lock, [this] {
                return lightmapPending.load() || !lightmapThreadRunning;
            });
            if (!lightmapThreadRunning) break;
            lightmapPending = false;
        }

        LightmapRequest req = lightmapRequest;
        lm.compute(world, req.camTileX, req.camTileY, req.visX, req.visY);

        lightmapBack.pixels  = lm.pixels;
        lightmapBack.width   = lm.width;
        lightmapBack.height  = lm.height;
        lightmapBack.originX = lm.originX;
        lightmapBack.originY = lm.originY;
        lightmapReady = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// =========================================================================
// Visibility
// =========================================================================

bool Game::isChunkVisible(int chunkX, int chunkY, const CameraParams &cam) const {
    float chunkWorldSize = world.chunks.getChunkSize() * Constants::TileSize;

    float minX = chunkX * chunkWorldSize;
    float minY = chunkY * chunkWorldSize;
    float maxX = minX + chunkWorldSize;
    float maxY = minY + chunkWorldSize;

    float camMinX = cam.position.x - cam.visibleWidth / 2.0f;
    float camMinY = cam.position.y - cam.visibleHeight / 2.0f;
    float camMaxX = cam.position.x + cam.visibleWidth / 2.0f;
    float camMaxY = cam.position.y + cam.visibleHeight / 2.0f;

    return maxX >= camMinX && minX <= camMaxX &&
           maxY >= camMinY && minY <= camMaxY;
}