#include "ARUI/Language/Parser.hpp"
#include <cctype>
#include <pugixml.hpp>
#include <string>

namespace ARUI::Language {
namespace {
std::string Trim(std::string_view value);

std::optional<LNodeType> NodeType(std::string_view name) {
  if (name == "surface") return LNodeType::Surface;
  if (name == "group") return LNodeType::Group;
  if (name == "row") return LNodeType::Row;
  if (name == "column") return LNodeType::Column;
  if (name == "stack") return LNodeType::Stack;
  if (name == "text") return LNodeType::Text;
  if (name == "button") return LNodeType::Button;
  if (name == "panel") return LNodeType::Panel;
  return std::nullopt;
}

bool HasOnlyKnownElements(pugi::xml_node node) {
  if (node.type() == pugi::node_element && !NodeType(node.name()))
    return false;
  for (auto child : node.children())
    if (!HasOnlyKnownElements(child))
      return false;
  return true;
}

AttributeValue ReadAttribute(std::string_view name, std::string value) {
  if (name == "disabled")
    return value == "true";
  if (name == "state")
    return StateReference{std::move(value)};
  if (name == "action")
    return ActionReference{std::move(value)};
  return value;
}

LNode ReadNode(pugi::xml_node source) {
  LNode result{.type = *NodeType(source.name())};
  for (auto attribute : source.attributes())
    result.attributes.insert_or_assign(
        attribute.name(), ReadAttribute(attribute.name(), attribute.value()));
  for (auto child : source.children()) {
    if (child.type() == pugi::node_element)
      result.children.push_back(ReadNode(child));
    else if (child.type() == pugi::node_pcdata ||
             child.type() == pugi::node_cdata) {
      auto text = Trim(child.value());
      if (!text.empty())
        result.children.push_back(LText(std::move(text)));
    }
  }
  return result;
}

std::string Trim(std::string_view value) {
  auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) return {};
  auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1));
}

class StyleParser {
public:
  explicit StyleParser(std::string_view input) : input(input) {}

  ParseResult<StyleSheet> Parse() {
    ParseResult<StyleSheet> result;
    while (SkipSpaceAndComments(result)) {
      if (position == input.size()) break;
      auto selectorStart = position;
      while (position < input.size() && input[position] != '{') ++position;
      if (position == input.size()) {
        Error(result, selectorStart, "expected '{' after selector");
        break;
      }
      StyleRule rule;
      rule.selector = Trim(input.substr(selectorStart, position++ - selectorStart));
      if (rule.selector.empty()) {
        Error(result, selectorStart, "empty selector");
        break;
      }
      bool closed = false;
      while (SkipSpaceAndComments(result)) {
        if (position == input.size()) break;
        if (input[position] == '}') {
          ++position;
          closed = true;
          break;
        }
        auto propertyStart = position;
        while (position < input.size() && input[position] != ':' &&
               input[position] != '}' && input[position] != ';') ++position;
        if (position == input.size() || input[position] != ':') {
          Error(result, propertyStart, "expected ':' after property");
          break;
        }
        Declaration declaration;
        declaration.property = Trim(input.substr(propertyStart, position++ - propertyStart));
        if (declaration.property.empty()) {
          Error(result, propertyStart, "empty property");
          break;
        }
        auto valueStart = position;
        int braces = 0, parens = 0;
        char quote = 0;
        bool escaped = false;
        for (; position < input.size(); ++position) {
          char c = input[position];
          if (quote) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == quote) quote = 0;
          } else if (c == '"' || c == '\'') quote = c;
          else if (c == '{') ++braces;
          else if (c == '(') ++parens;
          else if (c == ')') {
            if (parens == 0) { Error(result, position, "unmatched ')'"); break; }
            --parens;
          } else if (c == '}') {
            if (braces == 0 && parens == 0) break;
            if (braces > 0) --braces;
          } else if (c == ';' && braces == 0 && parens == 0) break;
        }
        if (!result) break;
        if (quote || braces || parens) {
          Error(result, valueStart, "unterminated style value");
          break;
        }
        declaration.value = Trim(input.substr(valueStart, position - valueStart));
        if (declaration.value.empty()) {
          Error(result, valueStart, "empty value");
          break;
        }
        rule.declarations.push_back(std::move(declaration));
        if (position < input.size() && input[position] == ';') ++position;
      }
      if (!result) break;
      if (!closed) {
        Error(result, selectorStart, "unterminated style rule");
        break;
      }
      result.value.rules.push_back(std::move(rule));
    }
    return result;
  }

private:
  bool SkipSpaceAndComments(ParseResult<StyleSheet> &result) {
    while (position < input.size()) {
      if (std::isspace(static_cast<unsigned char>(input[position]))) { ++position; continue; }
      if (input.substr(position, 2) == "/*") {
        auto end = input.find("*/", position + 2);
        if (end == std::string_view::npos) {
          Error(result, position, "unterminated comment");
          return false;
        }
        position = end + 2;
        continue;
      }
      break;
    }
    return true;
  }
  void Error(ParseResult<StyleSheet> &result, std::size_t offset, std::string message) {
    Diagnostic diagnostic;
    for (std::size_t i = 0; i < offset && i < input.size(); ++i) {
      if (input[i] == '\n') { ++diagnostic.line; diagnostic.column = 1; }
      else ++diagnostic.column;
    }
    diagnostic.message = std::move(message);
    result.diagnostics.push_back(std::move(diagnostic));
  }
  std::string_view input;
  std::size_t position = 0;
};
} // namespace

ParseResult<Document> ParseMarkup(std::string_view source) {
  ParseResult<Document> result;
  pugi::xml_document xml;
  auto status = xml.load_buffer(source.data(), source.size());
  if (!status) {
    Diagnostic diagnostic;
    for (std::ptrdiff_t i = 0; i < status.offset && i < static_cast<std::ptrdiff_t>(source.size()); ++i) {
      if (source[i] == '\n') { ++diagnostic.line; diagnostic.column = 1; }
      else ++diagnostic.column;
    }
    diagnostic.message = status.description();
    result.diagnostics.push_back(std::move(diagnostic));
    return result;
  }
  if (!xml.document_element() || !HasOnlyKnownElements(xml.document_element())) {
    result.diagnostics.push_back(
        {.message = "unknown or missing ARUI root element"});
    return result;
  }
  result.value.root = ReadNode(xml.document_element());
  return result;
}

ParseResult<StyleSheet> ParseStyles(std::string_view source) {
  return StyleParser(source).Parse();
}
} // namespace ARUI::Language
