#include "ARUI/Render/Font.hpp"

#include <algorithm>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <memory>
#include <stdexcept>
#include <string>

#ifndef ARUI_DEFAULT_FONT_PATH
#define ARUI_DEFAULT_FONT_PATH ""
#endif

namespace ARUI::Render {
namespace {
struct LibraryDeleter {
  void operator()(FT_LibraryRec_ *library) const {
    if (library) FT_Done_FreeType(library);
  }
};
struct FaceDeleter {
  void operator()(FT_FaceRec_ *face) const {
    if (face) FT_Done_Face(face);
  }
};

std::vector<FT_ULong> DecodeUtf8(std::string_view text) {
  std::vector<FT_ULong> codepoints;
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i]);
    FT_ULong value{};
    std::size_t count{};
    if (lead < 0x80) { value = lead; count = 1; }
    else if ((lead & 0xE0) == 0xC0) { value = lead & 0x1F; count = 2; }
    else if ((lead & 0xF0) == 0xE0) { value = lead & 0x0F; count = 3; }
    else if ((lead & 0xF8) == 0xF0) { value = lead & 0x07; count = 4; }
    else { value = 0xFFFD; count = 1; }
    if (i + count > text.size()) { codepoints.push_back(0xFFFD); break; }
    bool valid = true;
    for (std::size_t continuation = 1; continuation < count; ++continuation) {
      const auto byte = static_cast<unsigned char>(text[i + continuation]);
      if ((byte & 0xC0) != 0x80) { valid = false; break; }
      value = (value << 6) | (byte & 0x3F);
    }
    codepoints.push_back(valid ? value : 0xFFFD);
    i += valid ? count : 1;
  }
  return codepoints;
}
} // namespace

std::filesystem::path DefaultFontFile() {
  return std::filesystem::path{ARUI_DEFAULT_FONT_PATH};
}

RasterizedText RasterizeText(std::string_view text,
                             const std::filesystem::path &fontFile) {
  if (text.empty()) return {};
  FT_Library rawLibrary{};
  if (FT_Init_FreeType(&rawLibrary))
    throw std::runtime_error("FreeType initialization failed");
  std::unique_ptr<FT_LibraryRec_, LibraryDeleter> library{rawLibrary};
  FT_Face rawFace{};
  const std::string path = fontFile.string();
  if (path.empty() || FT_New_Face(library.get(), path.c_str(), 0, &rawFace))
    throw std::runtime_error("Could not load Noto Sans font: " + path);
  std::unique_ptr<FT_FaceRec_, FaceDeleter> face{rawFace};
  if (FT_Set_Pixel_Sizes(face.get(), 0, GlyphPixelHeight))
    throw std::runtime_error("Could not set FreeType pixel size");

  const auto codepoints = DecodeUtf8(text);
  int width = 0;
  for (const FT_ULong codepoint : codepoints) {
    if (FT_Load_Char(face.get(), codepoint, FT_LOAD_DEFAULT)) continue;
    width += static_cast<int>(face->glyph->advance.x >> 6);
  }
  const int ascender = static_cast<int>(face->size->metrics.ascender >> 6);
  const int descender = static_cast<int>(-(face->size->metrics.descender >> 6));
  const int height = std::max(1, ascender + descender);
  if (width <= 0) return {};

  RasterizedText result{.width = static_cast<uint32_t>(width),
                        .height = static_cast<uint32_t>(height),
                        .pixels = std::vector<std::byte>(
                            static_cast<std::size_t>(width * height))};
  int pen = 0;
  for (const FT_ULong codepoint : codepoints) {
    if (FT_Load_Char(face.get(), codepoint, FT_LOAD_RENDER)) continue;
    const FT_GlyphSlot glyph = face->glyph;
    const int destinationX = pen + glyph->bitmap_left;
    const int destinationY = ascender - glyph->bitmap_top;
    for (unsigned row = 0; row < glyph->bitmap.rows; ++row) {
      for (unsigned column = 0; column < glyph->bitmap.width; ++column) {
        const int x = destinationX + static_cast<int>(column);
        const int y = destinationY + static_cast<int>(row);
        if (x < 0 || y < 0 || x >= width || y >= height) continue;
        const auto source = glyph->bitmap.buffer[
            row * static_cast<unsigned>(std::abs(glyph->bitmap.pitch)) + column];
        result.pixels[static_cast<std::size_t>(y * width + x)] =
            static_cast<std::byte>(source);
      }
    }
    pen += static_cast<int>(glyph->advance.x >> 6);
  }
  return result;
}

} // namespace ARUI::Render
