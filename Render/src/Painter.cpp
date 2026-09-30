#include "ARUI/Render/Painter.hpp"
#include <stdexcept>
#include <utility>

namespace ARUI::Render {
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

const Language::PainterStyleSchema &
DefaultPainter::StyleSchema() const noexcept { return schema_; }

void DefaultPainter::Paint(const PaintNode &node,
                           const ComputedPainterStyle &style,
                           PaintContext &context) const {
  Language::Color color{{1.0F, 1.0F, 1.0F, 1.0F}};
  if (const auto found = style.find("background-color"); found != style.end())
    if (const auto *specified = std::get_if<Language::Color>(&found->second))
      color = *specified;
  context.DrawShape({.geometry = RectangleShape{.size = node.size},
                     .material = {.color = color},
                     .transform = {.localToWorld = node.localToWorld}});
}

void RegisterDefaultPainter(PainterRegistry &registry) {
  registry.Register("default", std::make_shared<DefaultPainter>());
}
} // namespace ARUI::Render
