#include "VISR/Runtime/Painters/VisrFlatPainter.hpp"

#include "VISR/Runtime/Painter/Commons/Layout.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <variant>

namespace VISR::Runtime {
namespace {

constexpr float NodeDepth = 0.0001F;

const Language::Color *
ColorProperty(const Language::PainterStyleProperties &properties,
              std::string_view name) noexcept {
  const auto found = properties.find(std::string{name});
  return found == properties.end()
             ? nullptr
             : std::get_if<Language::Color>(&found->second);
}

const Language::Length *
LengthProperty(const Language::PainterStyleProperties &properties,
               std::string_view name) noexcept {
  const auto found = properties.find(std::string{name});
  return found == properties.end()
             ? nullptr
             : std::get_if<Language::Length>(&found->second);
}

const std::string *
StringProperty(const Language::PainterStyleProperties &properties,
               std::string_view name) noexcept {
  const auto found = properties.find(std::string{name});
  return found == properties.end() ? nullptr
                                   : std::get_if<std::string>(&found->second);
}

Path RectanglePath(glm::vec2 size) {
  const glm::vec2 half = size * 0.5F;
  Path path;
  path.Move({-half.x, half.y})
      .Line({half.x, half.y})
      .Line({half.x, -half.y})
      .Line({-half.x, -half.y})
      .Close();
  return path;
}

void PaintNode(const PaintTreeContext &context,
               const PainterCommons::LayoutNode &layout,
               const Language::PainterStyleProperties *rootProperties,
               std::string_view inheritedFontFamily, float depth) {
  const RNode *node = context.tree.Get(layout.id);
  if (!node || !node->visible)
    return;

  const auto &properties =
      rootProperties ? *rootProperties : node->style.painterProperties;
  const auto *fontFamilyProperty = StringProperty(properties, "font-family");
  const std::string_view fontFamily =
      fontFamilyProperty ? std::string_view{*fontFamilyProperty}
                         : inheritedFontFamily;
  const glm::vec2 size = layout.bounds.Size();
  const glm::vec2 center =
      (layout.bounds.minimum + layout.bounds.maximum) * 0.5F;
  const glm::mat4 centerToWorld =
      context.localToWorld *
      glm::translate(glm::mat4{1.0F}, {center.x, center.y, depth});

  if (node->type == Language::LNodeType::Panel) {
    if (const auto *fill = ColorProperty(properties, "fill"))
      context.draw.DrawShape({.geometry = RectangleShape{.size = size},
                              .material = {.color = *fill},
                              .transform = {.localToWorld = centerToWorld}});

    const auto *stroke = ColorProperty(properties, "stroke");
    const auto *strokeWidth = LengthProperty(properties, "stroke-width");
    if (stroke && strokeWidth && strokeWidth->Value() > 0.0)
      context.draw.StrokePath(
          {.path = RectanglePath(size),
           .style = {.color = *stroke, .width = *strokeWidth},
           .transform = {.localToWorld = centerToWorld}});
  } else if (node->type == Language::LNodeType::Text) {
    const auto found = node->attributes.find("text");
    const auto *text = found == node->attributes.end()
                           ? nullptr
                           : std::get_if<std::string>(&found->second);
    if (text && !text->empty()) {
      const glm::vec2 origin{layout.contentBounds.minimum.x,
                             layout.contentBounds.maximum.y};
      context.draw.DrawText(
          {.text = *text,
           .fontSize = layout.fontSize,
           .fontFamily = std::string{fontFamily},
           .transform = {.localToWorld =
                             context.localToWorld *
                             glm::translate(glm::mat4{1.0F},
                                            {origin.x, origin.y, depth})}});
    }
  }

  for (const auto &child : layout.children)
    PaintNode(context, child, nullptr, fontFamily, depth + NodeDepth);
}

} // namespace

void VisrFlatPainter::Paint(
    const PaintTreeContext &context,
    const Language::PainterStyleProperties &properties) const {
  const RNode *root = context.tree.Get(context.root);
  if (!root || !root->visible || context.extent.x <= 0.0F ||
      context.extent.y <= 0.0F)
    return;

  PainterCommons::LayoutDefaults defaults{
      .padding = Language::DefaultLayoutParameters.padding,
      .gap = Language::DefaultLayoutParameters.gap,
      .fontSize = Language::DefaultLayoutParameters.fontSize,
      .fontFamily = std::string{Language::DefaultLayoutParameters.fontFamily}};
  if (const auto *fontFamily = StringProperty(properties, "font-family"))
    defaults.fontFamily = *fontFamily;

  const glm::vec2 half = context.extent * 0.5F;
  const auto layout =
      PainterCommons::LayoutTree(context.tree, context.root,
                                 {.minimum = -half, .maximum = half}, defaults);
  PaintNode(context, layout, &properties, defaults.fontFamily, 0.0F);
}

const Language::PainterStyleSchema &
VisrFlatPainter::StyleSchema() const noexcept {
  static const Language::PainterStyleSchema schema{{
      {"fill", Language::PainterStyleType::Color, false, true},
      {"stroke", Language::PainterStyleType::Color, false, true},
      {"stroke-width", Language::PainterStyleType::Length, false, false},
      {"font-family", Language::PainterStyleType::String, true, false},
  }};
  return schema;
}

void RegisterVisrFlatPainter(PainterRegistry &registry) {
  auto painter = std::make_shared<VisrFlatPainter>();
  registry.Register(std::string{DefaultPainterName}, painter);
  registry.Register("default", std::move(painter));
}

} // namespace VISR::Runtime
