#pragma once

#include "VISR/Language/node.hpp"

#include <string>
#include <string_view>

namespace VISR::Presentation::Wire {
std::string EncodeNode(const Language::LNode &node);
Language::LNode DecodeNode(std::string_view json);
std::string EncodeStyle(const Language::Style &style);
Language::Style DecodeStyle(std::string_view json);
} // namespace VISR::Presentation::Wire
