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


struct LayoutParameters {
  Length fontSize{5.0, LengthUnit::Millimeter};
};

inline constexpr LayoutParameters DefaultLayoutParameters{};

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

  // Inherited. ResolveFontSize applies the project default when unspecified.
  std::optional<Length> fontSize;

  // Surface-local translation, resolved before the surface is painted.
  std::optional<Length> xOffset;
  std::optional<Length> yOffset;
  std::optional<Length> zOffset;

  // Local Euler rotation of a surface, applied after its anchor orientation.
  std::optional<Angle> xRotation;
  std::optional<Angle> yRotation;
  std::optional<Angle> zRotation;

  std::optional<std::string> shape;
  std::optional<Length> radius;
  std::optional<Angle> arc;

  // Painter selection may inherit. Painter-property inheritance is declared
  // by the selected painter's schema.
  std::optional<PainterReference> painter;
  PainterStyleProperties painterProperties;
};

[[nodiscard]] constexpr Length
ResolveFontSize(const Style &style,
                Length inherited = DefaultLayoutParameters.fontSize) noexcept {
  return style.fontSize.value_or(inherited);
}

[[nodiscard]] PainterStyleType TypeOf(const PainterStyleValue &value) noexcept;

} // namespace ARUI::Language
