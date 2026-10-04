#pragma once
#include "VISR/Language/Document.hpp"
#include <string>

namespace VISR::Language {
std::string SerializeMarkup(const Document &document);
std::string SerializeStyles(const StyleSheet &sheet);
} // namespace VISR::Language
