#pragma once

#include "ARUI/Render/Painter.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"

namespace ARUI::Render {

struct SurfacePaintView {
  glm::mat4 localToClip{1.0F};
  float pixelsPerMeter{1.0F};
};

// Paints one complete runtime surface through the registered node painters.
void PaintRuntimeSurface(PaintContext &context,
                         const PainterRegistry &painters,
                         const Runtime::RuntimeTree &runtime,
                         Runtime::NodeID surface,
                         const SurfacePaintView &view);

} // namespace ARUI::Render
