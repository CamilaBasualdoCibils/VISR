#pragma once

#include "ARUI/Render/RenderObjects.hpp"

namespace ARUI::Render {

class RenderGraph;

class IRenderer {
public:
  virtual ~IRenderer() = default;

  virtual void BeginFrame() = 0;
  virtual void Submit(const SurfaceRenderObject &surface) = 0;
  virtual void Submit(const ShapeRenderObject &shape) = 0;
  virtual void Submit(const CurveRenderObject &curve) = 0;
  virtual void Submit(const TextRenderObject &text) = 0;
  virtual void Submit(const MeshRenderObject &mesh) = 0;
  virtual void BuildRenderGraph(RenderGraph &graph) = 0;
  virtual void EndFrame() = 0;
};

} // namespace ARUI::Render
