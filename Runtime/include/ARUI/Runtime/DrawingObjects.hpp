#pragma once

#include "ARUI/Language/AttributeValue.hpp"
#include "ARUI/Language/Color.hpp"
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string>
#include <variant>
#include <vector>

namespace ARUI::Runtime {

struct Transform {
  glm::mat4 localToWorld{1.0F};
};

struct MoveTo {
  glm::vec2 point;
};
struct LineTo {
  glm::vec2 point;
};
struct QuadraticTo {
  glm::vec2 control, end;
};
struct CubicTo {
  glm::vec2 control1, control2, end;
};
struct ArcTo {
  glm::vec2 radii;
  float rotationRadians{};
  bool largeArc{};
  bool sweep{};
  glm::vec2 end;
};
struct ClosePath {};

using PathCommand =
    std::variant<MoveTo, LineTo, QuadraticTo, CubicTo, ArcTo, ClosePath>;

struct Path {
  std::vector<PathCommand> commands;

  Path &Move(glm::vec2 point) {
    commands.emplace_back(MoveTo{point});
    return *this;
  }
  Path &Line(glm::vec2 point) {
    commands.emplace_back(LineTo{point});
    return *this;
  }
  Path &Quadratic(glm::vec2 control, glm::vec2 end) {
    commands.emplace_back(QuadraticTo{control, end});
    return *this;
  }
  Path &Cubic(glm::vec2 control1, glm::vec2 control2, glm::vec2 end) {
    commands.emplace_back(CubicTo{control1, control2, end});
    return *this;
  }
  Path &Arc(glm::vec2 radii, float rotationRadians, bool largeArc, bool sweep,
            glm::vec2 end) {
    commands.emplace_back(ArcTo{radii, rotationRadians, largeArc, sweep, end});
    return *this;
  }
  Path &Close() {
    commands.emplace_back(ClosePath{});
    return *this;
  }

  [[nodiscard]] bool IsClosed() const noexcept {
    return !commands.empty() &&
           std::holds_alternative<ClosePath>(commands.back());
  }
};

struct NoFill {};
struct SolidFill {
  Language::Color color;
};
using Fill = std::variant<NoFill, SolidFill>;

struct FillStyle {
  Fill fill{NoFill{}};
};

enum class LineJoin { Miter, Bevel, Round };
enum class LineCap { Butt, Square, Round };

struct StrokeStyle {
  Language::Color color{{0.0F, 0.0F, 0.0F, 1.0F}};
  Language::Length width{};
  LineJoin join{LineJoin::Miter};
  LineCap cap{LineCap::Butt};
};

struct FillPathRenderObject {
  Path path;
  FillStyle style;
  Transform transform;
};

struct StrokePathRenderObject {
  Path path;
  StrokeStyle style;
  Transform transform;
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

using SurfaceGeometry = std::variant<PlaneSurface, CylinderSurface,
                                     SphereSurface, BezierSurface, MeshSurface>;

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
  float fontSizePixels{16.0F};
  // Bitmap pixels per local-space unit; keeps glyph geometry view-independent.
  float pixelsPerUnit{1.0F};
  std::string fontFamily{"Noto Sans"};
  Material material;
  Transform transform;
};

struct MeshRenderObject {
  MeshGeometry geometry;
  Material material;
  Transform transform;
};

class DrawingObjectSink {
public:
  virtual ~DrawingObjectSink() = default;
  virtual void FillPath(const FillPathRenderObject &) = 0;
  virtual void StrokePath(const StrokePathRenderObject &) = 0;
  virtual void Submit(const SurfaceRenderObject &) = 0;
  virtual void Submit(const ShapeRenderObject &) = 0;
  virtual void Submit(const CurveRenderObject &) = 0;
  virtual void Submit(const TextRenderObject &) = 0;
  virtual void Submit(const MeshRenderObject &) = 0;
};

} // namespace ARUI::Runtime
