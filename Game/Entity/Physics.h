#pragma once

#include "Player.h"
#include "../Game.h"

void applyMovement(Player &player, const InputState &input, float deltaTime);
void resolveCollisions(Player &player, World &world, float deltaTime);