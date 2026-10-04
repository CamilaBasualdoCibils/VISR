#pragma once
#include "VISR/Language/Diagnostic.hpp"
#include "VISR/Language/Document.hpp"
#include <filesystem>
#include <string_view>

namespace VISR::Language {
ParseResult<Document> ParseMarkup(std::string_view source);
ParseResult<Document> ParseMarkupFile(const std::filesystem::path &path);
inline ParseResult<VISRDocument> ParseVISR(std::string_view source) {
  return ParseMarkup(source);
}
inline ParseResult<VISRDocument> ParseVISRFile(const std::filesystem::path &path) {
  return ParseMarkupFile(path);
}
ParseResult<StyleSheet> ParseStyles(std::string_view source);
} // namespace VISR::Language
