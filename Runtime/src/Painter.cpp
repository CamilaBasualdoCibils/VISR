#include "ARUI/Runtime/Painter.hpp"
#include <stdexcept>
#include <utility>

namespace ARUI::Runtime {
namespace {
Path PanelPath(glm::vec2 size) {
  const glm::vec2 half = size * 0.5F;
  Path path;
  path.Move({-half.x, half.y})
      .Line({half.x, half.y})
      .Line({half.x, -half.y})
      .Line({-half.x, -half.y})
      .Close();
  return path;
}

const Language::Color *ColorProperty(const ComputedPainterStyle &style,
                                     std::string_view name) {
  const auto found = style.find(std::string{name});
  return found == style.end() ? nullptr
                              : std::get_if<Language::Color>(&found->second);
}
} // namespace

void PainterRegistry::Register(std::string name,
                               std::shared_ptr<const IPainter> painter) {
  if (name.empty())
    throw std::invalid_argument("painter name must not be empty");
  if (!painter)
    throw std::invalid_argument("painter must not be null");
  if (!painters_.emplace(std::move(name), std::move(painter)).second)
    throw std::logic_error("a painter with that name is already registered");
}

const IPainter *PainterRegistry::Find(std::string_view name) const noexcept {
  const auto found = painters_.find(std::string{name});
  return found == painters_.end() ? nullptr : found->second.get();
}

const Language::PainterStyleSchema &FlatPainter::StyleSchema() const noexcept {
  return schema_;
}

void FlatPainter::Paint(const PaintNode &node,
                        const ComputedPainterStyle &style,
                        PaintContext &context) const {
  // Text remains a separate semantic glyph/bitmap submission from the earlier
  // text implementation; the panel branch below never paints children.
  if (node.type == Language::LNodeType::Text) {
    if (!node.attributes)
      return;
    const auto found = node.attributes->find("text");
    if (found == node.attributes->end())
      return;
    const auto *textValue = std::get_if<std::string>(&found->second);
    if (!textValue || textValue->empty())
      return;
    context.DrawText(
        {.text = *textValue,
         .fontSizePixels = node.fontSizePixels,
         .pixelsPerUnit = node.pixelsPerUnit,
         .fontFamily = std::string{node.fontFamily},
         .transform = {.localToWorld = node.contentOriginToWorld}});
    return;
  }

  if (node.type != Language::LNodeType::Panel)
    return;

  const auto *fill = ColorProperty(style, "fill");
  const auto *stroke = ColorProperty(style, "stroke");
  const auto widthValue = style.find("stroke-width");
  const auto *strokeWidth =
      widthValue == style.end()
          ? nullptr
          : std::get_if<Language::Length>(&widthValue->second);
  if (!fill && (!stroke || !strokeWidth || strokeWidth->value <= 0.0))
    return;

  const Path path = PanelPath(node.size);
  if (fill)
    context.FillPath({.path = path,
                      .style = {.fill = SolidFill{*fill}},
                      .transform = {.localToWorld = node.localToWorld}});
  if (stroke && strokeWidth && strokeWidth->value > 0.0)
    context.StrokePath({.path = path,
                        .style = {.color = *stroke, .width = *strokeWidth},
                        .transform = {.localToWorld = node.localToWorld}});
}

void RegisterFlatPainter(PainterRegistry &registry) {
  auto painter = std::make_shared<FlatPainter>();
  registry.Register(std::string{DefaultPainterName}, painter);
  registry.Register("default", std::move(painter));
}
} // namespace ARUI::Runtime
