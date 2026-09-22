#pragma once

#include "ARUI/Language/AttributeValue.hpp"
#include "ARUI/Language/Color.hpp"
#include <boost/describe/class.hpp>
#include <optional>
namespace ARUI::Language {
struct Style {
  // layout
  std::optional<Length> width;
  std::optional<Length> height;
  std::optional<Length> minWidth;
  std::optional<Length> minHeight;
  std::optional<Length> maxWidth;
  std::optional<Length> maxHeight;

  std::optional<Length> gap;
  std::optional<Length> padding;

  // appearance
  std::optional<Color> color;
  std::optional<Color> backgroundColor;
  std::optional<double> opacity;

  // etc...
};
BOOST_DESCRIBE_STRUCT(Style, (),
                      (width, height, minWidth, minHeight, maxWidth, maxHeight,
                       gap, padding, color, backgroundColor, opacity))

} // namespace ARUI::Language
