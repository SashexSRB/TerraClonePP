#include "Vertex.h"

std::array<glm::vec2, 4> getTexCoords(int x, int y, int atlasSize = 256,
                                      int tileSize = 8) {
  float step = static_cast<float>(tileSize) / static_cast<float>(atlasSize);

  float u0 = x * step;
  float v0 = y * step;
  float u1 = u0 + step;
  float v1 = v0 + step;

  return {{
      {u0, v0}, // top-left
      {u1, v0}, // top-right
      {u1, v1}, // bottom-right
      {u0, v1}, // bottom-left
  }};
}
