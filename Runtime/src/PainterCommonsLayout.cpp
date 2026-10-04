#include "VISR/Runtime/Painter/Commons/Layout.hpp"

#include <algorithm>
#include <cmath>

namespace VISR::Runtime::PainterCommons {
namespace {

float ResolveMeters(const Language::Length &value, float reference,
                    const Language::Length &fallback) noexcept {
  const auto resolve = [reference](const Language::Length &length,
                                   float fallbackValue) noexcept {
    if (length.IsAbsolute())
      return static_cast<float>(length.As(Language::LengthUnit::Meter).Value());
    if (length.IsPercent())
      return reference * static_cast<float>(length.Value() * 0.01);
    return fallbackValue;
  };
  const float fallbackMeters = resolve(fallback, 0.0F);
  return std::max(0.0F, resolve(value, fallbackMeters));
}

Language::Length ResolveFontSize(const RNode &node,
                                 const Language::Length &inherited) noexcept {
  if (!node.style.fontSize || node.style.fontSize->IsAuto())
    return inherited;
  if (node.style.fontSize->IsAbsolute())
    return *node.style.fontSize;
  const float inheritedMeters = ResolveMeters(
      inherited, 0.0F, Language::Length{0.0, Language::LengthUnit::Meter});
  return Language::Length{inheritedMeters * node.style.fontSize->Value() * 0.01,
                          Language::LengthUnit::Meter};
}

LayoutBounds SafeBounds(LayoutBounds bounds) noexcept {
  if (!std::isfinite(bounds.minimum.x) || !std::isfinite(bounds.minimum.y) ||
      !std::isfinite(bounds.maximum.x) || !std::isfinite(bounds.maximum.y))
    return {};
  bounds.maximum = {std::max(bounds.maximum.x, bounds.minimum.x),
                    std::max(bounds.maximum.y, bounds.minimum.y)};
  return bounds;
}

LayoutNode LayoutNodeRecursive(const RuntimeTree &tree, const RNode &node,
                               LayoutBounds bounds,
                               const LayoutDefaults &defaults,
                               const Language::Length &inheritedFontSize) {
  bounds = SafeBounds(bounds);
  const glm::vec2 size = bounds.Size();
  const float padding =
      ResolveMeters(node.style.padding.value_or(defaults.padding),
                    std::min(size.x, size.y), defaults.padding);
  const glm::vec2 inset{std::min(padding, size.x * 0.5F),
                        std::min(padding, size.y * 0.5F)};
  const LayoutBounds content{bounds.minimum + inset, bounds.maximum - inset};
  const Language::Length fontSize = ResolveFontSize(node, inheritedFontSize);

  LayoutNode result{.id = node.id,
                    .bounds = bounds,
                    .contentBounds = content,
                    .fontSize = fontSize,
                    .fontFamily = defaults.fontFamily};
  if (node.children.empty() || !node.visible)
    return result;

  const float count = static_cast<float>(node.children.size());
  const glm::vec2 innerSize = content.Size();
  const float reference =
      node.type == Language::LNodeType::Row ? innerSize.x : innerSize.y;
  float gap = ResolveMeters(node.style.gap.value_or(defaults.gap), reference,
                            defaults.gap);
  if (node.children.size() > 1)
    gap = std::min(gap, reference / (count - 1.0F));
  else
    gap = 0.0F;

  result.children.reserve(node.children.size());
  for (std::size_t i = 0; i < node.children.size(); ++i) {
    const RNode *child = tree.Get(node.children[i]);
    if (!child)
      continue;
    LayoutBounds childBounds = content;
    if (node.type == Language::LNodeType::Row) {
      const float width =
          std::max(0.0F, (innerSize.x - gap * (count - 1.0F)) / count);
      childBounds.minimum.x += static_cast<float>(i) * (width + gap);
      childBounds.maximum.x = childBounds.minimum.x + width;
    } else if (node.type != Language::LNodeType::Stack) {
      const float height =
          std::max(0.0F, (innerSize.y - gap * (count - 1.0F)) / count);
      childBounds.maximum.y -= static_cast<float>(i) * (height + gap);
      childBounds.minimum.y = childBounds.maximum.y - height;
    }
    result.children.push_back(
        LayoutNodeRecursive(tree, *child, childBounds, defaults, fontSize));
  }
  return result;
}

} // namespace

LayoutNode LayoutTree(const RuntimeTree &tree, NodeID root,
                      const LayoutBounds &rootBounds,
                      const LayoutDefaults &defaults) {
  const RNode *node = tree.Get(root);
  if (!node)
    return {};
  return LayoutNodeRecursive(tree, *node, rootBounds, defaults,
                             defaults.fontSize);
}

} // namespace VISR::Runtime::PainterCommons
