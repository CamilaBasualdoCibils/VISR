#pragma once
#include "VISR/Language/node.hpp"
#include <string>
#include <vector>

namespace VISR::Language {
struct EmbeddedScript {
  std::string type;
  std::string source;
  bool operator==(const EmbeddedScript &) const = default;
};

struct LDocument {
  std::vector<LNode> surfaces;
  std::vector<std::string> stylesheets;
  std::vector<EmbeddedScript> scripts;
};
using Document = LDocument;
using VISRDocument = LDocument;

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
} // namespace VISR::Language
