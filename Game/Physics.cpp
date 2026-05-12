#include "Physics.h"

void applyMovement(Player &player, const InputState &input, float deltaTime) {
    player.velocity.x = 0.0f;

    if (input.left)  player.velocity.x = -player.moveSpeed;
    if (input.right) player.velocity.x = player.moveSpeed;

    if (input.jump && player.isGrounded) {
        player.velocity.y = -player.jumpSpeed;
        player.isGrounded = false;
    }

    player.velocity.y += 800.f * deltaTime;
}

void resolveCollisions(Player &player, World &world, float deltaTime) {
    constexpr float playerSize = 32.0f;

    auto isTileSolid = [&](int x, int y) -> bool {
        if (x < 0 || x >= world.getWidth() || y < 0 || y >= world.getHeight()) return true;

        Tile &tile = world.getTile(x, y);

        return (tile.isActive && TileRegistry::tileTypes[tile.tileId].isSolid) ||
               (tile.wallId != 0 && TileRegistry::wallTypes[tile.wallId].isSolid);
    };

    // Horizontal collision
    glm::vec2 newPos = player.position;
    newPos.x += player.velocity.x * deltaTime;

    int topTileY    = static_cast<int>(player.position.y / playerSize);
    int bottomTileY = static_cast<int>((player.position.y + playerSize - 1) / playerSize);

    if (player.velocity.x > 0.0f) {
        int rightTileX = static_cast<int>((newPos.x + playerSize - 1) / playerSize);
        bool collision = false;

        for (int y = topTileY; y <= bottomTileY; ++y)
            if (isTileSolid(rightTileX, y)) { collision = true; break; }

        if (collision) {
            newPos.x = rightTileX * playerSize - playerSize;
            player.velocity.x = 0.0f;
        }
    } else if (player.velocity.x < 0.0f) {
        int leftTileX = static_cast<int>(newPos.x / playerSize);
        bool collision = false;

        for (int y = topTileY; y <= bottomTileY; ++y)
            if (isTileSolid(leftTileX, y)) { collision = true; break; }

        if (collision) {
            newPos.x = (leftTileX + 1) * playerSize;
            player.velocity.x = 0.0f;
        }
    }
    player.position.x = newPos.x;

    // Ground check
    int leftTileX  = static_cast<int>(player.position.x / playerSize);
    int rightTileX = static_cast<int>((player.position.x + playerSize - 1) / playerSize);

    if (player.isGrounded && player.velocity.y >= 0.0f) {
        float bottomY = player.position.y + playerSize + 0.001f;
        int botTileY  = static_cast<int>(bottomY / playerSize);
        bool onGround = false;

        for (int x = leftTileX; x <= rightTileX; ++x)
            if (isTileSolid(x, botTileY)) { onGround = true; break; }

        if (onGround) {
            player.velocity.y = 0.0f;
        } else {
            player.isGrounded = false;
        }
    }

    // Vertical collision
    newPos.y = player.position.y + player.velocity.y * deltaTime;

    if (player.velocity.y > 0.0f) {
        int botTileY = static_cast<int>((newPos.y + playerSize - 1) / playerSize);
        bool collision = false;

        for (int x = leftTileX; x <= rightTileX; ++x)
            if (isTileSolid(x, botTileY)) { collision = true; break; }

        if (collision) {
            newPos.y = botTileY * playerSize - playerSize;
            player.velocity.y = 0.0f;
            player.isGrounded = true;
        } else {
            player.isGrounded = false;
            player.position.y = newPos.y;
        }
    } else if (player.velocity.y < 0.0f) {
        int topTileYCheck = static_cast<int>(newPos.y / playerSize);
        bool collision = false;

        for (int x = leftTileX; x <= rightTileX; ++x)
            if (isTileSolid(x, topTileYCheck)) { collision = true; break; }

        if (collision) {
            newPos.y = (topTileYCheck + 1) * playerSize;
            player.velocity.y = 0.0f;
        } else {
            player.position.y = newPos.y;
        }
    }
    player.position.y = newPos.y;

    // Clamp
    player.position.x = std::max(0.0f, std::min(player.position.x, world.getWidth() * playerSize - playerSize));
    player.position.y = std::max(0.0f, std::min(player.position.y, world.getHeight() * playerSize - playerSize));
}