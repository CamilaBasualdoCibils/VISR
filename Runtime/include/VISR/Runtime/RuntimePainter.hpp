#pragma once

#include "VISR/Runtime/IPainter.hpp"
#include "VISR/Runtime/RuntimeTree.hpp"

namespace VISR::Runtime {

struct SurfacePaintView {
  // Surface-local physical coordinates to world space. Camera transforms belong
  // to the renderer and must not be supplied to a painter.
  glm::mat4 localToWorld{1.0F};
};

void PaintRuntimeSurface(PaintContext &context, const PainterRegistry &painters,
                         const RuntimeTree &runtime, NodeID surface,
                         const SurfacePaintView &view);

} // namespace VISR::Runtime
