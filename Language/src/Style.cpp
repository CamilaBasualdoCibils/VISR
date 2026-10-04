#include "VISR/Language/Style.hpp"

#include <algorithm>
#include <type_traits>

namespace VISR::Language {

const PainterStylePropertyDescriptor *
PainterStyleSchema::Find(std::string_view name) const noexcept {
  const auto found = std::find_if(
      properties.begin(), properties.end(),
      [name](const auto &property) { return property.name == name; });
  return found == properties.end() ? nullptr : &*found;
}

bool PainterStyleSchema::Accepts(std::string_view name,
                                 const PainterStyleValue &value) const noexcept {
  const auto *property = Find(name);
  if (property == nullptr) return false;
  if (property->acceptsNone)
    if (const auto *text = std::get_if<std::string>(&value))
      return *text == "none";
  return property->type == TypeOf(value);
}

PainterStyleType TypeOf(const PainterStyleValue &value) noexcept {
  return std::visit(
      []<typename T>(const T &) {
        if constexpr (std::is_same_v<T, bool>)
          return PainterStyleType::Bool;
        else if constexpr (std::is_same_v<T, int64_t>)
          return PainterStyleType::Int;
        else if constexpr (std::is_same_v<T, double>)
          return PainterStyleType::Double;
        else if constexpr (std::is_same_v<T, std::string>)
          return PainterStyleType::String;
        else if constexpr (std::is_same_v<T, Length>)
          return PainterStyleType::Length;
        else if constexpr (std::is_same_v<T, Angle>)
          return PainterStyleType::Angle;
        else
          return PainterStyleType::Color;
      },
      value);
}

} // namespace VISR::Language
