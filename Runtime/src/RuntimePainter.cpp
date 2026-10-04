#include "ARUI/Runtime/RuntimePainter.hpp"
#include "ARUI/Language/AttributeValue.hpp"
#include <algorithm>
#include <glm/ext/matrix_transform.hpp>
#include <string_view>
namespace ARUI::Runtime {
/* float LengthInMeters(Language::Length length, float fallback) noexcept {
  switch (length.unit) {
  case Language::LengthUnit::Millimeter:
    return static_cast<float>(length.value * 0.001);
  case Language::LengthUnit::Centimeter:
    return static_cast<float>(length.value * 0.01);
  case Language::LengthUnit::Meter:
    return static_cast<float>(length.value);
  case Language::LengthUnit::Percent:
    return fallback * static_cast<float>(length.value * 0.01);
  case Language::LengthUnit::Auto:
    return fallback;
  }
  return fallback;
} */
/* 
namespace {
void PaintRuntimeNode(PaintContext &context, const PainterRegistry &painters,
                      const RuntimeTree &runtime, NodeID id, glm::vec2 minimum,
                      glm::vec2 maximum, const SurfacePaintView &view,
                      Language::Length inheritedFontSize, float depth) {
  const auto *node = runtime.Get(id);
  if (!node || !node->visible)
    return;
  const std::string painterName = node->style.painter
                                      ? node->style.painter->name
                                      : std::string{DefaultPainterName};
  const IPainter *painter = painters.Find(painterName);
  if (!painter)
    painter = painters.Find(std::string{DefaultPainterName});
  if (!painter)
    return;

  const glm::vec2 size = glm::max(maximum - minimum, glm::vec2{0.000001F});
  const glm::vec2 center = (minimum + maximum) * 0.5F;
  const auto centerTransform =
      view.localToClip *
      glm::translate(glm::mat4{1.0F}, {center.x, center.y, depth});
  const auto contentTransform =
      view.localToClip *
      glm::translate(glm::mat4{1.0F}, {minimum.x, maximum.y, depth});
  painter->Paint({.type = node->type,
                  .size = size,
                  .localToWorld = centerTransform,
                  .contentOriginToWorld = contentTransform,
                  .attributes = &node->attributes,

                  .fontSize =,
                  .fontFamily = Language::DefaultLayoutParameters.fontFamily},
                 node->style.painterProperties, context);

  if (node->children.empty())
    return;
  const float padding =
      node->style.padding.value_or(Language::DefaultLayoutParameters.padding)
          .As(Language::LengthUnit::Meter)
          .Value();
  const float gap =
      node->style.gap.value_or(Language::DefaultLayoutParameters.gap)
          .As(Language::LengthUnit::Meter)
          .Value();
  const glm::vec2 innerMin = minimum + glm::vec2{padding};
  const glm::vec2 innerMax = maximum - glm::vec2{padding};
  const glm::vec2 innerSize =
      glm::max(innerMax - innerMin, glm::vec2{0.000001F});
  const float count = static_cast<float>(node->children.size());
  for (std::size_t i = 0; i < node->children.size(); ++i) {
    glm::vec2 childMin = innerMin;
    glm::vec2 childMax = innerMax;
    if (node->type == Language::LNodeType::Row) {
      const float width = (innerSize.x - gap * (count - 1.0F)) / count;
      childMin.x += static_cast<float>(i) * (width + gap);
      childMax.x = childMin.x + width;
    } else if (node->type != Language::LNodeType::Stack) {
      const float height = (innerSize.y - gap * (count - 1.0F)) / count;
      childMax.y -= static_cast<float>(i) * (height + gap);
      childMin.y = childMax.y - height;
    }
    PaintRuntimeNode(context, painters, runtime, node->children[i], childMin,
                     childMax, view,
                     Language::ResolveFontSize(node->style, inheritedFontSize),
                     depth + 0.0001F);
  }
}
} // namespace */

/* void PaintRuntimeSurface(PaintContext &context, const PainterRegistry &painters,
                         const RuntimeTree &runtime, NodeID surfaceID,
                         const SurfacePaintView &view) {
  const auto *surface = runtime.Get(surfaceID);
  if (!surface || surface->type != Language::LNodeType::Surface ||
      !surface->visible)
    return;
  const float width =
      surface->style.width.As(Language::LengthUnit::Meter).Value();
  const float height =
      surface->style.height.As(Language::LengthUnit::Meter).Value();
  if (width <= 0.0F || height <= 0.0F)
    return;
  PaintRuntimeNode(context, painters, runtime, surfaceID,
                   {-width * 0.5F, -height * 0.5F},
                   {width * 0.5F, height * 0.5F}, view,
                   Language::DefaultLayoutParameters.fontSize, 0.0F);
} */
void PaintRuntimeSurface(
    PaintContext& context,
    const PainterRegistry& painters,
    const RuntimeTree& runtime,
    NodeID surfaceID,
    const SurfacePaintView& view)
{
    const RNode* surface = runtime.Get(surfaceID);

    if (!surface ||
        !surface->visible ||
        surface->type != Language::LNodeType::Surface)
    {
        return;
    }

    const std::string_view painterName =
        surface->style.painter
            ? surface->style.painter->name
            : DefaultPainterName;

    const IPainter* painter = painters.Find(painterName);

    if (!painter)
        painter = painters.Find(DefaultPainterName);

    if (!painter)
        return;

    const glm::vec2 extent{
        surface->style.width
            .As(Language::LengthUnit::Meter)
            .Value(),

        surface->style.height
            .As(Language::LengthUnit::Meter)
            .Value()
    };

    if (extent.x <= 0.0f || extent.y <= 0.0f)
        return;

    painter->Paint(
        PaintTreeContext{
            .draw         = context,
            .tree         = runtime,
            .root         = surfaceID,
            .localToWorld = view.localToClip,
            .extent       = extent,
        },
        surface->style.painterProperties
    );
}
} // namespace ARUI::Runtime
