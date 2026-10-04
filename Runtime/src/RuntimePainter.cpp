#include "VISR/Runtime/RuntimePainter.hpp"

namespace VISR::Runtime {

void PaintRuntimeSurface(PaintContext &context, const PainterRegistry &painters,
                         const RuntimeTree &runtime, NodeID surfaceID,
                         const SurfacePaintView &view) {
  const RNode *surface = runtime.Get(surfaceID);
  if (!surface || !surface->visible ||
      surface->type != Language::LNodeType::Surface)
    return;

  const std::string_view painterName = surface->style.painter
                                           ? surface->style.painter->name
                                           : DefaultPainterName;
  const IPainter *painter = painters.Find(painterName);
  if (!painter)
    painter = painters.Default();
  if (!painter)
    return;

  if (!surface->style.width.IsAbsolute() || !surface->style.height.IsAbsolute())
    return;
  const glm::vec2 extent{
      surface->style.width.As(Language::LengthUnit::Meter).Value(),
      surface->style.height.As(Language::LengthUnit::Meter).Value()};
  if (extent.x <= 0.0F || extent.y <= 0.0F)
    return;

  painter->Paint({.draw = context,
                  .tree = runtime,
                  .root = surfaceID,
                  .localToWorld = view.localToWorld,
                  .extent = extent},
                 surface->style.painterProperties);
}

} // namespace VISR::Runtime
