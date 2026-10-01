#include "ARUI/Language/Parser.hpp"
#include <cctype>
#include <charconv>
#include <fstream>
#include <pugixml.hpp>
#include <sstream>
#include <string>
#include <system_error>

namespace ARUI::Language {
namespace {
std::string Trim(std::string_view value);

std::optional<LNodeType> NodeType(std::string_view name) {
  if (name == "group") return LNodeType::Group;
  if (name == "row") return LNodeType::Row;
  if (name == "column") return LNodeType::Column;
  if (name == "stack") return LNodeType::Stack;
  if (name == "text") return LNodeType::Text;
  if (name == "image") return LNodeType::Image;
  if (name == "panel") return LNodeType::Panel;
  return std::nullopt;
}

template <typename T>
void Error(ParseResult<T> &result, std::string_view source,
           pugi::xml_node node, std::string message) {
  Diagnostic diagnostic{.message = std::move(message)};
  const auto offset = node.offset_debug();
  for (std::ptrdiff_t i = 0; i < offset &&
       i < static_cast<std::ptrdiff_t>(source.size()); ++i) {
    if (source[i] == '\n') { ++diagnostic.line; diagnostic.column = 1; }
    else ++diagnostic.column;
  }
  result.diagnostics.push_back(std::move(diagnostic));
}

std::optional<Length> ParsePhysicalLength(std::string_view input,
                                          std::string &reason) {
  const auto value = Trim(input);
  std::size_t unitStart = 0;
  while (unitStart < value.size() &&
         (std::isdigit(static_cast<unsigned char>(value[unitStart])) ||
          value[unitStart] == '+' || value[unitStart] == '-' ||
          value[unitStart] == '.')) ++unitStart;
  if (unitStart == 0) {
    reason = "malformed physical length '" + value + "'";
    return std::nullopt;
  }
  double number{};
  const auto numberText = std::string_view(value).substr(0, unitStart);
  const auto parsed = std::from_chars(numberText.data(),
                                      numberText.data() + numberText.size(), number);
  if (parsed.ec != std::errc{} || parsed.ptr != numberText.data() + numberText.size() ||
      number <= 0.0) {
    reason = "malformed physical length '" + value + "'";
    return std::nullopt;
  }
  const auto unit = std::string_view(value).substr(unitStart);
  if (unit == "mm") return Length{number, LengthUnit::Millimeter};
  if (unit == "cm") return Length{number, LengthUnit::Centimeter};
  if (unit == "m") return Length{number, LengthUnit::Meter};
  reason = "unsupported physical length unit in '" + value +
           "' (expected mm, cm, or m)";
  return std::nullopt;
}

bool ReadNode(ParseResult<Document> &parsed, std::string_view input,
              pugi::xml_node source, LNode &result) {
  const auto type = NodeType(source.name());
  if (!type) {
    Error(parsed, input, source,
          "unknown ARUI node <" + std::string(source.name()) + ">");
    return false;
  }
  result.type = *type;
  for (auto attribute : source.attributes()) {
    const std::string name = attribute.name();
    const std::string value = attribute.value();
    const bool common = name == "class" || name == "behavior";
    const bool named = name == "name" &&
        (*type == LNodeType::Group || *type == LNodeType::Panel);
    const bool image = *type == LNodeType::Image &&
        (name == "src" || name == "alt");
    if (name == "state" && *type == LNodeType::Text)
      result.attributes.insert_or_assign(name, StateReference{value});
    else if (common || named || image)
      result.attributes.insert_or_assign(name, value);
    else {
      Error(parsed, input, source, "invalid attribute '" + name +
            "' on <" + source.name() + "> (value '" + value + "')");
      return false;
    }
  }
  std::string text;
  for (auto child : source.children()) {
    if (child.type() == pugi::node_element) {
      if (*type == LNodeType::Text || *type == LNodeType::Image) {
        Error(parsed, input, child, "<" + std::string(source.name()) +
              "> cannot contain child elements");
        return false;
      }
      LNode node{};
      if (!ReadNode(parsed, input, child, node)) return false;
      result.children.push_back(std::move(node));
    } else if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
      text += child.value();
    }
  }
  text = Trim(text);
  if (*type == LNodeType::Text) {
    if (!text.empty()) result.attributes.insert_or_assign("text", std::move(text));
  } else if (!text.empty()) {
    Error(parsed, input, source, "unexpected text content in <" +
          std::string(source.name()) + ">");
    return false;
  }
  return true;
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
  const auto status = xml.load_buffer(source.data(), source.size());
  if (!status) {
    Diagnostic diagnostic;
    for (std::ptrdiff_t i = 0; i < status.offset &&
         i < static_cast<std::ptrdiff_t>(source.size()); ++i) {
      if (source[i] == '\n') { ++diagnostic.line; diagnostic.column = 1; }
      else ++diagnostic.column;
    }
    diagnostic.message = std::string("XML syntax error: ") + status.description();
    result.diagnostics.push_back(std::move(diagnostic));
    return result;
  }

  const auto root = xml.document_element();
  std::size_t rootElements = 0;
  for (auto child : xml.children())
    if (child.type() == pugi::node_element) ++rootElements;
  if (!root || rootElements != 1 || std::string_view(root.name()) != "arui") {
    result.diagnostics.push_back({.message =
        "ARUI document root must be exactly one <arui> element"});
    return result;
  }
  if (root.first_attribute()) {
    Error(result, source, root, "<arui> does not accept attributes");
    return result;
  }

  for (auto child : root.children()) {
    if (child.type() == pugi::node_comment || child.type() == pugi::node_declaration)
      continue;
    if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
      if (!Trim(child.value()).empty()) {
        Error(result, source, child, "unexpected text directly inside <arui>");
        return result;
      }
      continue;
    }
    const std::string_view name = child.name();
    if (name == "style") {
      if (child.first_attribute()) {
        Error(result, source, child, "<style> does not accept attributes");
        return result;
      }
      std::string stylesheet;
      for (auto content : child.children()) {
        if (content.type() == pugi::node_pcdata || content.type() == pugi::node_cdata)
          stylesheet += content.value();
        else if (content.type() == pugi::node_element) {
          Error(result, source, content, "<style> cannot contain markup elements");
          return result;
        }
      }
      result.value.stylesheets.push_back(std::move(stylesheet));
    } else if (name == "script") {
      if (const auto external = child.attribute("src")) {
        Error(result, source, child, "External ARUI scripts are not supported yet: " +
              std::string(external.value()));
        return result;
      }
      for (auto attribute : child.attributes()) {
        if (std::string_view(attribute.name()) != "type") {
          Error(result, source, child, "invalid attribute '" +
                std::string(attribute.name()) + "' on <script>");
          return result;
        }
      }
      EmbeddedScript script{.type = child.attribute("type").value()};
      for (auto content : child.children()) {
        if (content.type() == pugi::node_pcdata || content.type() == pugi::node_cdata)
          script.source += content.value();
        else if (content.type() == pugi::node_element) {
          Error(result, source, content, "<script> cannot contain markup elements");
          return result;
        }
      }
      result.value.scripts.push_back(std::move(script));
    } else if (name == "surface") {
      LNode surface{.type = LNodeType::Surface};
      for (auto attribute : child.attributes()) {
        const std::string attributeName = attribute.name();
        if (attributeName == "width" || attributeName == "height") continue;
        if (attributeName == "anchor" || attributeName == "class" ||
            attributeName == "behavior")
          surface.attributes.insert_or_assign(attributeName,
                                               std::string(attribute.value()));
        else {
          Error(result, source, child, "invalid attribute '" + attributeName +
                "' on <surface> (value '" + attribute.value() + "')");
          return result;
        }
      }
      for (const auto dimension : {std::string_view("width"), std::string_view("height")}) {
        const auto attribute = child.attribute(dimension.data());
        if (!attribute) {
          Error(result, source, child, "<surface> is missing required '" +
                std::string(dimension) + "' attribute");
          return result;
        }
        std::string reason;
        const auto length = ParsePhysicalLength(attribute.value(), reason);
        if (!length) {
          Error(result, source, child, "invalid <surface> " +
                std::string(dimension) + ": " + reason);
          return result;
        }
        if (dimension == "width") surface.style.width = *length;
        else surface.style.height = *length;
      }
      std::string surfaceText;
      for (auto content : child.children()) {
        if (content.type() == pugi::node_element) {
          LNode node{};
          if (!ReadNode(result, source, content, node)) return result;
          surface.children.push_back(std::move(node));
        } else if (content.type() == pugi::node_pcdata || content.type() == pugi::node_cdata) {
          surfaceText += content.value();
        }
      }
      if (!Trim(surfaceText).empty()) {
        Error(result, source, child, "unexpected text content in <surface>");
        return result;
      }
      result.value.surfaces.push_back(std::move(surface));
    } else {
      Error(result, source, child, "unsupported top-level ARUI element <" +
            std::string(child.name()) + ">");
      return result;
    }
  }
  return result;
}

ParseResult<Document> ParseMarkupFile(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
    return {.diagnostics = {{.message = "unable to read ARUI file: " + path.string()}}};
  std::ostringstream contents;
  contents << stream.rdbuf();
  return ParseMarkup(contents.str());
}

ParseResult<StyleSheet> ParseStyles(std::string_view source) {
  return StyleParser(source).Parse();
}
} // namespace ARUI::Language
