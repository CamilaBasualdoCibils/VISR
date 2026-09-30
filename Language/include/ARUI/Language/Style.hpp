#pragma once

#include "ARUI/Language/AttributeValue.hpp"
#include "ARUI/Language/Color.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ARUI::Language {


struct PainterReference {
  std::string name;
  bool operator==(const PainterReference &) const = default;
};

using PainterStyleValue =
    std::variant<bool, int64_t, double, std::string, Length, Angle, Color>;
using PainterStyleProperties =
    std::unordered_map<std::string, PainterStyleValue>;

enum class PainterStyleType { Bool, Int, Double, String, Length, Angle, Color };

struct PainterStylePropertyDescriptor {
  std::string name;
  PainterStyleType type{};
  bool inherited{};
};

struct PainterStyleSchema {
  std::vector<PainterStylePropertyDescriptor> properties;

  [[nodiscard]] const PainterStylePropertyDescriptor *
  Find(std::string_view name) const noexcept;
  [[nodiscard]] bool Accepts(std::string_view name,
                             const PainterStyleValue &value) const noexcept;
};

// These are specified declarations, not computed values. A future resolver
// must apply cascade and inheritance policy per property.
struct Style {
  Length width;
  Length height;
  Length minWidth;
  Length minHeight;
  Length maxWidth;
  Length maxHeight;

  std::optional<Length> margin;
  std::optional<Length> padding;
  std::optional<Length> gap;

  // Layout resolves spatial presentation before a painter is invoked.
  std::optional<Length> xOffset;
  std::optional<Length> yOffset;
  std::optional<Length> zOffset;

  std::optional<std::string> shape;
  std::optional<Length> radius;
  std::optional<Angle> arc;

  // Painter selection may inherit. Painter-property inheritance is declared
  // by the selected painter's schema.
  std::optional<PainterReference> painter;
  PainterStyleProperties painterProperties;
};

[[nodiscard]] PainterStyleType TypeOf(const PainterStyleValue &value) noexcept;

} // namespace ARUI::Language
