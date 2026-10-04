#pragma once
#include "ARUI/Language/AttributeValue.hpp"
#include "ARUI/Language/Style.hpp"
#include "ARUI/Language/node.hpp"
#include "ARUI/Runtime/DrawingObjects.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ARUI::Runtime {
inline constexpr std::string_view DefaultPainterName{"arui-flat-painter"};
// Immutable output of layout. Painters do not compute bounds or placement.

using ComputedPainterStyle = Language::PainterStyleProperties;

class PaintContext {
public:
  explicit PaintContext(DrawingObjectSink &renderer) : renderer_(renderer) {}
  void FillPath(const FillPathRenderObject &v) { renderer_.FillPath(v); }
  void StrokePath(const StrokePathRenderObject &v) { renderer_.StrokePath(v); }
  void DrawSurface(const SurfaceRenderObject &v) { renderer_.Submit(v); }
  void DrawShape(const ShapeRenderObject &v) { renderer_.Submit(v); }
  void DrawCurve(const CurveRenderObject &v) { renderer_.Submit(v); }
  void DrawText(const TextRenderObject &v) { renderer_.Submit(v); }
  void DrawMesh(const MeshRenderObject &v) { renderer_.Submit(v); }

private:
  DrawingObjectSink &renderer_;
};
struct PaintTreeContext {
  PaintContext &draw;

  const RuntimeTree &tree;
  NodeID root;

  glm::mat4 localToWorld;

  // Physical extent of the root surface.
  glm::vec2 extent;

  // Possibly runtime/environment information:
  float pixelsPerMeter;
  double time;
};
class IPainter {
public:
  virtual ~IPainter() = default;
  [[nodiscard]] virtual const Language::PainterStyleSchema &
  StyleSchema() const noexcept = 0;
  virtual void
  Paint(const PaintTreeContext &context,
        const Language::PainterStyleProperties &properties) const = 0;
};

class PainterRegistry {
public:
  void Register(std::string name, std::shared_ptr<const IPainter> painter);
  [[nodiscard]] const IPainter *Find(std::string_view name) const noexcept;
  [[nodiscard]] const IPainter *Default() const noexcept {
    return Find(DefaultPainterName);
  }

private:
  std::unordered_map<std::string, std::shared_ptr<const IPainter>> painters_;
};
/*
class FlatPainter final : public IPainter {
public:
  [[nodiscard]] const Language::PainterStyleSchema &
  StyleSchema() const noexcept override;
  void Paint(const PaintNode &, const ComputedPainterStyle &,
             PaintContext &) const override;

private:
  Language::PainterStyleSchema schema_{{
      {"fill", Language::PainterStyleType::Color, false, true},
      {"stroke", Language::PainterStyleType::Color, false, true},
      {"stroke-width", Language::PainterStyleType::Length, false},
      {"font-size", Language::PainterStyleType::Length, true},
      {"font-family", Language::PainterStyleType::String, true},
  }};
};

using DefaultPanelPainter = FlatPainter;

void RegisterFlatPainter(PainterRegistry &registry); */
} // namespace ARUI::Runtime
