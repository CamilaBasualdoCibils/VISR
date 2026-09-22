#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
namespace ARUI::Language {

struct StateReference {
    std::string value;
};

struct ActionReference {
    std::string value;
};

enum class LengthUnit {
    Pixel,
    Millimeter,
    Centimeter,
    Meter,
    Percent
};
struct Length {
  double value;
  LengthUnit unit;
};
enum class AngleUnit {
    Degree,
    Radian
};
struct Angle {
  double value;
  AngleUnit unit;
};
enum class AttributeType {
    Bool,
    Int,
    Double,
    String,
    Length,
    Angle,
    StateReference,
    ActionReference
};
using AttributeValue = std::variant<bool, int64_t, double, std::string,

                                    Length, Angle,

                                    StateReference, ActionReference>;
using Attributes = std::unordered_map<std::string, AttributeValue>;
} // namespace ARUI::Language