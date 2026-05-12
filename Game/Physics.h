#pragma once

#include "Game.h"
#include "Player.h"
#include "World.h"

void applyMovement(Player &player, const InputState &input, float deltaTime);
void resolveCollisions(Player &player, World &world, float deltaTime);