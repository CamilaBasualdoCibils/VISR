#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace VISR::Render {

inline constexpr uint32_t GlyphPixelHeight = 64;

struct RasterizedText {
  uint32_t width{};
  uint32_t height{};
  std::vector<std::byte> pixels;

  [[nodiscard]] bool Empty() const noexcept {
    return width == 0 || height == 0 || pixels.empty();
  }
};

// Rasterizes a single UTF-8 line at the renderer's fixed glyph resolution.
[[nodiscard]] RasterizedText
RasterizeText(std::string_view text, const std::filesystem::path &fontFile);

[[nodiscard]] std::filesystem::path DefaultFontFile();

} // namespace VISR::Render
