#pragma once
#include "ARUI/Language/Diagnostic.hpp"
#include "ARUI/Language/Document.hpp"
#include <filesystem>
#include <string_view>

namespace ARUI::Language {
ParseResult<Document> ParseMarkup(std::string_view source);
ParseResult<Document> ParseMarkupFile(const std::filesystem::path &path);
inline ParseResult<ARUIDocument> ParseARUI(std::string_view source) {
  return ParseMarkup(source);
}
inline ParseResult<ARUIDocument> ParseARUIFile(const std::filesystem::path &path) {
  return ParseMarkupFile(path);
}
ParseResult<StyleSheet> ParseStyles(std::string_view source);
} // namespace ARUI::Language
