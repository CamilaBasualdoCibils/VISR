#pragma once

#include <array>
#include <cmath>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string_view>

namespace ARUI::Language {

// Canonical colors are linear floating-point RGBA and may exceed 1.0 for HDR.
struct Color {
  glm::vec4 rgba;
  bool operator==(const Color &) const = default;
};

[[nodiscard]] inline float SRGBToLinear(float channel) noexcept {
  return channel <= 0.04045F
             ? channel / 12.92F
             : std::pow((channel + 0.055F) / 1.055F, 2.4F);
}

[[nodiscard]] inline Color LinearColorFromSRGB(float red, float green,
                                                float blue,
                                                float alpha = 1.0F) noexcept {
  return {{SRGBToLinear(red), SRGBToLinear(green), SRGBToLinear(blue), alpha}};
}

[[nodiscard]] inline std::optional<Color>
ParseSRGBHexColor(std::string_view value) noexcept {
  if (value.size() != 7 && value.size() != 9) return std::nullopt;
  if (value.front() != '#') return std::nullopt;
  auto digit = [](char character) -> int {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
  };
  std::array<float, 4> channels{0.0F, 0.0F, 0.0F, 1.0F};
  const std::size_t count = value.size() == 9 ? 4 : 3;
  for (std::size_t i = 0; i < count; ++i) {
    const int high = digit(value[1 + i * 2]);
    const int low = digit(value[2 + i * 2]);
    if (high < 0 || low < 0) return std::nullopt;
    channels[i] = static_cast<float>(high * 16 + low) / 255.0F;
  }
  return LinearColorFromSRGB(channels[0], channels[1], channels[2], channels[3]);
}

} // namespace ARUI::Language
