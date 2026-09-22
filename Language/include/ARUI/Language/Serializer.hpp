#pragma once
#include "ARUI/Language/Document.hpp"
#include <string>

namespace ARUI::Language {
std::string SerializeMarkup(const Document &document);
std::string SerializeStyles(const StyleSheet &sheet);
} // namespace ARUI::Language
