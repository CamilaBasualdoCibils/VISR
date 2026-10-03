#pragma once
#include "ARUI/Language/Style.hpp"
#include "ARUI/Language/node.hpp"
#include "ARUI/Render/IRenderer.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ARUI::Render {
// Immutable output of layout. Painters do not compute bounds or placement.
struct PaintNode {
  Language::LNodeType type{};
  glm::vec2 size{};
  glm::mat4 localToWorld{1.0F};
  // Top-left content origin supplied by layout (positive Y points up).
  glm::mat4 contentOriginToWorld{1.0F};
  const Language::Attributes *attributes{};
  float fontSizePixels{16.0F};
  float pixelsPerUnit{1.0F};
  std::string_view fontFamily{"Noto Sans"};
};
using ComputedPainterStyle = Language::PainterStyleProperties;

class PaintContext {
public:
  explicit PaintContext(IRenderer &renderer) : renderer_(renderer) {}
  void FillPath(const FillPathRenderObject &v) { renderer_.FillPath(v); }
  void StrokePath(const StrokePathRenderObject &v) { renderer_.StrokePath(v); }
  void DrawSurface(const SurfaceRenderObject &v) { renderer_.Submit(v); }
  void DrawShape(const ShapeRenderObject &v) { renderer_.Submit(v); }
  void DrawCurve(const CurveRenderObject &v) { renderer_.Submit(v); }
  void DrawText(const TextRenderObject &v) { renderer_.Submit(v); }
  void DrawMesh(const MeshRenderObject &v) { renderer_.Submit(v); }
private:
  IRenderer &renderer_;
};

class IPainter {
public:
  virtual ~IPainter() = default;
  [[nodiscard]] virtual const Language::PainterStyleSchema &
  StyleSchema() const noexcept = 0;
  virtual void Paint(const PaintNode &, const ComputedPainterStyle &,
                     PaintContext &) const = 0;
};

class PainterRegistry {
public:
  void Register(std::string name, std::shared_ptr<const IPainter> painter);
  [[nodiscard]] const IPainter *Find(std::string_view name) const noexcept;
private:
  std::unordered_map<std::string, std::shared_ptr<const IPainter>> painters_;
};

class DefaultPainter final : public IPainter {
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

using DefaultPanelPainter = DefaultPainter;

void RegisterDefaultPainter(PainterRegistry &registry);
} // namespace ARUI::Render
