#pragma once

#include <glm/glm.hpp>

/**
 * Camera state used for rendering and world-space calculations.
 *
 * Defines the visible region of the world in world-space coordinates.
 */
struct CameraParams {
    glm::vec2 position;
    float visibleWidth;
    float visibleHeight;
};