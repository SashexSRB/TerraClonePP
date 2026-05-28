#include "Physics.h"
#include "Constants.h"
#include "Items/Item.h"

void applyMovement(Player &player, const InputState &input, float deltaTime) {
    player.velocity.x = 0.0f;
    if (input.left)  player.velocity.x = -player.moveSpeed;
    if (input.right) player.velocity.x = player.moveSpeed;

    // Initial jump
    if (input.jump && player.isGrounded) {
        player.velocity.y = -player.jumpSpeed;
        player.isGrounded = false;
        player.isJumping = true;
        player.jumpTimer = 0.0f;
    }

    // Variable jump (keep boosting upward while space held and within time limit
    if (input.jump && player.isJumping && !player.isGrounded) {
        player.jumpTimer += deltaTime;
        if (player.jumpTimer < player.maxJumpTime) {
            // Apply extra upward force proportional to remaining jump time
            float jumpForce = player.jumpSpeed * (1.0f - player.jumpTimer / player.maxJumpTime);
            player.velocity.y -= jumpForce * deltaTime;
        } else {
            player.isJumping = false;
        }
    }

    // Cancel jump boost when space released
    if (!input.jump) {
        if (player.isJumping) {
            // Cut velocity if still moving up
            if (player.velocity.y < 0.0f) player.velocity.y *= 0.5f;
            player.isJumping = false;
        }
    }

    // Reset jump state when grounded
    if (player.isGrounded) {
        player.isJumping = false;
        player.jumpTimer = 0.0f;
    }

    player.velocity.y += Constants::PlayerGravity * deltaTime;

    // Falling speed cap
    if (player.velocity.y > Constants::PlayerMaxVSpeed)
        player.velocity.y = Constants::PlayerMaxVSpeed;
}

void resolveCollisions(Player &player, World &world, float deltaTime) {
    constexpr float pw = Constants::PlayerWidth;
    constexpr float ph = Constants::PlayerHeight;

    auto isTileSolid = [&](int x, int y) -> bool {
        if (x < 0 || x >= world.getWidth() || y < 0 || y >= world.getHeight()) return true;

        Tile &tile = world.getTile(x, y);

        return (tile.isActive && Registry::get(tile.tileId).isSolid) ||
               (tile.wallId != 0 && Registry::get(tile.wallId).isSolid);
    };

    // Horizontal collision
    glm::vec2 newPos = player.position;
    newPos.x += player.velocity.x * deltaTime;

    int topTileY    = static_cast<int>((player.position.y + Constants::PlrHbOffsetH) / Constants::TileSize);
    int bottomTileY = static_cast<int>((player.position.y + Constants::PlrHbOffsetH + Constants::PlayerHBHeight - 1) / Constants::TileSize);

    if (player.velocity.x > 0.0f) {
        int rightTileX = static_cast<int>((newPos.x + Constants::PlrHbOffsetW + Constants::PlayerHBWidth - 1) / Constants::TileSize);
        bool collision = false;

        for (int y = topTileY; y <= bottomTileY; ++y)
            if (isTileSolid(rightTileX, y)) { collision = true; break; }

        if (collision) {
            newPos.x = rightTileX * Constants::TileSize - Constants::PlayerHBWidth - Constants::PlrHbOffsetW;
            player.velocity.x = 0.0f;
        }
    } else if (player.velocity.x < 0.0f) {
        int leftTileX = static_cast<int>((newPos.x + Constants::PlrHbOffsetW) / Constants::TileSize);
        bool collision = false;

        for (int y = topTileY; y <= bottomTileY; ++y)
            if (isTileSolid(leftTileX, y)) { collision = true; break; }

        if (collision) {
            newPos.x = (leftTileX + 1) * Constants::TileSize - Constants::PlrHbOffsetW;
            player.velocity.x = 0.0f;
        }
    }
    player.position.x = newPos.x;

    // Ground check
    int leftTileX  = static_cast<int>((player.position.x + Constants::PlrHbOffsetW) / Constants::TileSize);
    int rightTileX = static_cast<int>((player.position.x + Constants::PlrHbOffsetW + Constants::PlayerHBWidth - 1) / Constants::TileSize);

    if (player.isGrounded && player.velocity.y >= 0.0f) {
        int bottomTileY  = static_cast<int>((newPos.y + Constants::PlrHbOffsetH + Constants::PlayerHBHeight - 1) / Constants::TileSize);
        bool onGround = false;

        for (int x = leftTileX; x <= rightTileX; ++x)
            if (isTileSolid(x, bottomTileY)) { onGround = true; break; }

        if (onGround) {
            player.velocity.y = 0.0f;
        } else {
            player.isGrounded = false;
        }
    }

    // Vertical collision
    newPos.y = player.position.y + player.velocity.y * deltaTime;

    if (player.velocity.y > 0.0f) {
        int botTileY = static_cast<int>((newPos.y + ph - 1) / Constants::TileSize);
        bool collision = false;

        for (int x = leftTileX; x <= rightTileX; ++x)
            if (isTileSolid(x, botTileY)) { collision = true; break; }

        if (collision) {
            newPos.y = botTileY * Constants::TileSize - Constants::PlayerHBHeight - Constants::PlrHbOffsetH;
            player.velocity.y = 0.0f;
            player.isGrounded = true;
        } else {
            player.isGrounded = false;
            player.position.y = newPos.y;
        }
    } else if (player.velocity.y < 0.0f) {
        int topTileYCheck = static_cast<int>((newPos.y + Constants::PlrHbOffsetH) / Constants::TileSize);
        bool collision = false;

        for (int x = leftTileX; x <= rightTileX; ++x)
            if (isTileSolid(x, topTileYCheck)) { collision = true; break; }

        if (collision) {
            newPos.y = (topTileYCheck + 1) * Constants::TileSize - Constants::PlrHbOffsetH;
            player.velocity.y = 0.0f;
        } else {
            player.position.y = newPos.y;
        }
    }
    player.position.y = newPos.y;

    // Clamp
    player.position.x = std::max(0.0f, std::min(player.position.x, world.getWidth() * Constants::TileSize - pw));
    player.position.y = std::max(0.0f, std::min(player.position.y, world.getHeight() * Constants::TileSize - ph));
}