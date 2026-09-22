#pragma once
#include "ARUI/Language/Diagnostic.hpp"
#include "ARUI/Language/Document.hpp"
#include <string_view>

namespace ARUI::Language {
ParseResult<Document> ParseMarkup(std::string_view source);
ParseResult<StyleSheet> ParseStyles(std::string_view source);
} // namespace ARUI::Language
