#pragma once
#include <string>
#include <vector>

namespace ARUI::Language {
struct Attribute {
  std::string name, value;
  bool operator==(const Attribute &) const = default;
};
struct LNode {
  // Text nodes have an empty name and store their contents in text.
  std::string name, text;
  std::vector<Attribute> attributes;
  std::vector<LNode> children;
  bool operator==(const LNode &) const = default;
};
struct Document {
  LNode root;
  bool operator==(const Document &) const = default;
};
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
