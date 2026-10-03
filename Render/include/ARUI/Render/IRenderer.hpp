#pragma once

#include "ARUI/Runtime/DrawingObjects.hpp"

namespace ARUI::Render {
using namespace Runtime;

class RenderGraph;

class IRenderer : public Runtime::DrawingObjectSink {
public:
  virtual ~IRenderer() = default;

  virtual void BeginFrame() = 0;
  virtual void FillPath(const FillPathRenderObject &path) = 0;
  virtual void StrokePath(const StrokePathRenderObject &path) = 0;
  virtual void Submit(const SurfaceRenderObject &surface) = 0;
  virtual void Submit(const ShapeRenderObject &shape) = 0;
  virtual void Submit(const CurveRenderObject &curve) = 0;
  virtual void Submit(const TextRenderObject &text) = 0;
  virtual void Submit(const MeshRenderObject &mesh) = 0;
  virtual void BuildRenderGraph(RenderGraph &graph) = 0;
  virtual void EndFrame() = 0;
};

} // namespace ARUI::Render
