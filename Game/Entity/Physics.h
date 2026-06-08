#pragma once

#include "Player.h"
#include "Game.h"

/**
 * Applies player movement based on current input state.
 *
 * @param player Player state that will be modified (position/velocity)
 * @param input Current input snapshot used for movement decisions
 * @param deltaTime Frame time step used for frame-independent movement
 */
void applyMovement(Player &player, const InputState &input, float deltaTime);

/**
 * Resolves collisions between player and world geometry.
 *
 * @param player Player state that may be modified to resolve collisions
 * @param world World used for collision queries
 * @param deltaTime Frame time step used for physics resolution
 */
void resolveCollisions(Player &player, World &world, float deltaTime);