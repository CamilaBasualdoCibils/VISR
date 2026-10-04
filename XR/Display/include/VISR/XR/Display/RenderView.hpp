#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <string>
namespace VISR {
struct RenderView {
  std::string Name;
  glm::mat4 view;
  glm::mat4 projection;
  glm::ivec2 viewportSize;
};
};
