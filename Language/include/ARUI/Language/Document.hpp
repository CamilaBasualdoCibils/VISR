#pragma once
#include "ARUI/Language/node.hpp"
#include <string>
#include <vector>

namespace ARUI::Language {
struct LDocument { LNode root; };
using Document = LDocument;

struct Declaration {
  std::string property, value;
  bool operator==(const Declaration &) const = default;
};
struct StyleRule {
  std::string selector;
  std::vector<Declaration> declarations;
  bool operator==(const StyleRule &) const = default;
};
struct StyleSheet {
  std::vector<StyleRule> rules;
  bool operator==(const StyleSheet &) const = default;
};
} // namespace ARUI::Language
