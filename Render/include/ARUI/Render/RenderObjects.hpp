#pragma once

#include "ARUI/Language/Color.hpp"
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <variant>
#include <vector>

namespace ARUI::Render {

struct Transform {
  glm::mat4 localToWorld{1.0F};
};

struct Material {
  Language::Color color{{1.0F, 1.0F, 1.0F, 1.0F}};
};

enum class MeshTopology { Triangles, Lines, Points };

struct MeshGeometry {
  std::vector<glm::vec3> positions;
  std::vector<uint32_t> indices;
  MeshTopology topology{MeshTopology::Triangles};
};

struct PlaneSurface {
  glm::vec2 size{1.0F, 1.0F};
};

struct CylinderSurface {
  float radius{0.5F};
  float height{1.0F};
  float arcRadians{6.283185307F};
};

struct SphereSurface {
  float radius{0.5F};
};

struct BezierSurface {
  std::vector<glm::vec3> controlPoints;
  uint32_t uCount{};
  uint32_t vCount{};
};

struct MeshSurface {
  MeshGeometry mesh;
};

using SurfaceGeometry =
    std::variant<PlaneSurface, CylinderSurface, SphereSurface, BezierSurface,
                 MeshSurface>;

struct RectangleShape {
  glm::vec2 size{1.0F, 1.0F};
  float cornerRadius{};
};

struct EllipseShape {
  glm::vec2 radii{0.5F, 0.5F};
};

using ShapeGeometry = std::variant<RectangleShape, EllipseShape>;

struct CurveGeometry {
  std::vector<glm::vec3> points;
  bool closed{};
};

struct CurveStyle {
  Language::Color color{{1.0F, 1.0F, 1.0F, 1.0F}};
  float width{1.0F};
};

struct SurfaceRenderObject {
  SurfaceGeometry geometry;
  Material material;
  Transform transform;
};

struct ShapeRenderObject {
  ShapeGeometry geometry;
  Material material;
  Transform transform;
};

struct CurveRenderObject {
  CurveGeometry geometry;
  CurveStyle style;
  Transform transform;
};

struct TextRenderObject {
  std::string text;
  float fontSize{16.0F};
  Material material;
  Transform transform;
};

struct MeshRenderObject {
  MeshGeometry geometry;
  Material material;
  Transform transform;
};

} // namespace ARUI::Render
