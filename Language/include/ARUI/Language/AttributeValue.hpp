#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
namespace ARUI::Language {

struct StateReference {
  std::string value;
};

struct ActionReference {
  std::string value;
};

enum class LengthUnit { Auto, Millimeter, Centimeter, Meter, Percent };
struct Length {
  double value{};
  LengthUnit unit{LengthUnit::Auto};

  [[nodiscard]] bool IsAuto() const noexcept {
    return unit == LengthUnit::Auto;
  }

  [[nodiscard]] bool IsPercent() const noexcept {
    return unit == LengthUnit::Percent;
  }

  [[nodiscard]] bool IsAbsolute() const noexcept {
    return unit == LengthUnit::Millimeter || unit == LengthUnit::Centimeter ||
           unit == LengthUnit::Meter;
  }

  bool operator==(const Length &) const = default;
  [[nodiscard]] Length As(LengthUnit newUnit) const {
    if (unit == newUnit)
      return *this;

    if (!IsAbsolute() || newUnit == LengthUnit::Auto ||
        newUnit == LengthUnit::Percent) {
      throw std::runtime_error(
          "Cannot convert relative/auto length without context");
    }

    // Convert current value -> meters.
    double meters;

    switch (unit) {
    case LengthUnit::Millimeter:
      meters = value * 0.001;
      break;

    case LengthUnit::Centimeter:
      meters = value * 0.01;
      break;

    case LengthUnit::Meter:
      meters = value;
      break;

    default:
      std::unreachable();
    }

    // Convert meters -> requested unit.
    switch (newUnit) {
    case LengthUnit::Millimeter:
      return {meters / 0.001, newUnit};

    case LengthUnit::Centimeter:
      return {meters / 0.01, newUnit};

    case LengthUnit::Meter:
      return {meters, newUnit};

    default:
      std::unreachable();
    }
  }
  [[nodiscard]] Length Resolve(const Length &reference) const {
    switch (unit) {
    case LengthUnit::Millimeter:
    case LengthUnit::Centimeter:
    case LengthUnit::Meter:
      return *this;

    case LengthUnit::Percent: {
      if (!reference.IsAbsolute()) {
        throw std::runtime_error(
            "Percent requires an absolute reference length");
      }

      return Length{reference.value * (value / 100.0), reference.unit};
    }

    case LengthUnit::Auto:
      throw std::runtime_error(
          "Auto cannot be resolved without layout information");
    }

    std::unreachable();
  }
  double Value() const noexcept { return value; }
};
enum class AngleUnit { Degree, Radian };
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