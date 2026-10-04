#include "ARUI/Language/Parser.hpp"
#include <array>
#include <cctype>
#include <charconv>
#include <fstream>
#include <pugixml.hpp>
#include <span>
#include <sstream>
#include <string>
#include <system_error>

namespace ARUI::Language {
namespace {
std::string Trim(std::string_view value);

template <typename T>
void Error(ParseResult<T> &result, std::string_view source, pugi::xml_node node,
           std::string message) {
  Diagnostic diagnostic{.message = std::move(message)};
  const auto offset = node.offset_debug();
  for (std::ptrdiff_t i = 0;
       i < offset && i < static_cast<std::ptrdiff_t>(source.size()); ++i) {
    if (source[i] == '\n') {
      ++diagnostic.line;
      diagnostic.column = 1;
    } else
      ++diagnostic.column;
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
          value[unitStart] == '.'))
    ++unitStart;
  if (unitStart == 0) {
    reason = "malformed physical length '" + value + "'";
    return std::nullopt;
  }
  double number{};
  const auto numberText = std::string_view(value).substr(0, unitStart);
  const auto parsed = std::from_chars(
      numberText.data(), numberText.data() + numberText.size(), number);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != numberText.data() + numberText.size()) {
    reason = "malformed physical length '" + value + "'";
    return std::nullopt;
  }
  const auto unit = std::string_view(value).substr(unitStart);
  if (unit == "mm")
    return Length{number, LengthUnit::Millimeter};
  if (unit == "cm")
    return Length{number, LengthUnit::Centimeter};
  if (unit == "m")
    return Length{number, LengthUnit::Meter};
  reason = "unsupported physical length unit in '" + value +
           "' (expected mm, cm, or m)";
  return std::nullopt;
}

std::optional<Angle> ParsePhysicalAngle(std::string_view input,
                                         std::string &reason) {
  const auto value = Trim(input);
  std::size_t unitStart = 0;
  while (unitStart < value.size() &&
         (std::isdigit(static_cast<unsigned char>(value[unitStart])) ||
          value[unitStart] == '+' || value[unitStart] == '-' ||
          value[unitStart] == '.'))
    ++unitStart;
  if (unitStart == 0) {
    reason = "malformed physical angle '" + value + "'";
    return std::nullopt;
  }
  double number{};
  const auto numberText = std::string_view(value).substr(0, unitStart);
  const auto parsed = std::from_chars(
      numberText.data(), numberText.data() + numberText.size(), number);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != numberText.data() + numberText.size()) {
    reason = "malformed physical angle '" + value + "'";
    return std::nullopt;
  }
  const auto unit = std::string_view(value).substr(unitStart);
  if (unit == "deg")
    return Angle{number, AngleUnit::Degree};
  if (unit == "rad")
    return Angle{number, AngleUnit::Radian};
  reason = "unsupported physical angle unit in '" + value +
           "' (expected deg or rad)";
  return std::nullopt;
}
using AttributeParser = bool (*)(void *, std::string_view, std::string_view,
                                 std::string &);
struct AttributeSpec {
  std::string_view name;
  bool required;
  AttributeParser parser;
};
struct ElementSpec {
  std::string_view name;
  LNodeType type;
  std::span<const AttributeSpec> attributes;
};

bool ParseStringAttribute(void *target, std::string_view name,
                          std::string_view value, std::string &) {
  static_cast<LNode *>(target)->SetAttribute(std::string(name),
                                             std::string(value));
  return true;
}
bool ParseStateAttribute(void *target, std::string_view name,
                         std::string_view value, std::string &) {
  static_cast<LNode *>(target)->SetAttribute(
      std::string(name), StateReference{std::string(value)});
  return true;
}
bool ParseWidthAttribute(void *target, std::string_view, std::string_view value,
                         std::string &reason) {
  const auto length = ParsePhysicalLength(value, reason);
  if (!length)
    return false;
  static_cast<LNode *>(target)->style.width = *length;
  return true;
}
bool ParseHeightAttribute(void *target, std::string_view,
                          std::string_view value, std::string &reason) {
  const auto length = ParsePhysicalLength(value, reason);
  if (!length)
    return false;
  static_cast<LNode *>(target)->style.height = *length;
  return true;
}
bool ParseXOffsetAttribute(void *target, std::string_view,
                          std::string_view value, std::string &reason) {
  const auto length = ParsePhysicalLength(value, reason);
  if (!length)
    return false;
  static_cast<LNode *>(target)->style.xOffset = *length;
  return true;
}
bool ParseYOffsetAttribute(void *target, std::string_view,
                          std::string_view value, std::string &reason) {
  const auto length = ParsePhysicalLength(value, reason);
  if (!length)
    return false;
  static_cast<LNode *>(target)->style.yOffset = *length;
  return true;
}
bool ParseZOffsetAttribute(void *target, std::string_view,
                          std::string_view value, std::string &reason) {
  const auto length = ParsePhysicalLength(value, reason);
  if (!length)
    return false;
  static_cast<LNode *>(target)->style.zOffset = *length;
  return true;
}
bool ParseXRotationAttribute(void *target, std::string_view,
                            std::string_view value, std::string &reason) {
  const auto angle = ParsePhysicalAngle(value, reason);
  if (!angle)
    return false;
  static_cast<LNode *>(target)->style.xRotation = *angle;
  return true;
}
bool ParseYRotationAttribute(void *target, std::string_view,
                            std::string_view value, std::string &reason) {
  const auto angle = ParsePhysicalAngle(value, reason);
  if (!angle)
    return false;
  static_cast<LNode *>(target)->style.yRotation = *angle;
  return true;
}
bool ParseZRotationAttribute(void *target, std::string_view,
                            std::string_view value, std::string &reason) {
  const auto angle = ParsePhysicalAngle(value, reason);
  if (!angle)
    return false;
  static_cast<LNode *>(target)->style.zRotation = *angle;
  return true;
}
bool ParseScriptTypeAttribute(void *target, std::string_view,
                              std::string_view value, std::string &) {
  static_cast<EmbeddedScript *>(target)->type = value;
  return true;
}

constexpr AttributeSpec StringAttribute(std::string_view name,
                                        bool required = false) {
  return {name, required, ParseStringAttribute};
}
constexpr AttributeSpec StateAttribute(std::string_view name,
                                       bool required = false) {
  return {name, required, ParseStateAttribute};
}
constexpr AttributeSpec PhysicalLengthAttribute(std::string_view name,
                                                AttributeParser parser,
                                                bool required = false) {
  return {name, required, parser};
}
constexpr AttributeSpec PhysicalAngleAttribute(std::string_view name,
                                               AttributeParser parser,
                                               bool required = false) {
  return {name, required, parser};
}

constexpr auto CommonAttributes = std::to_array<AttributeSpec>(
    {StringAttribute("class"), StringAttribute("behavior")});
constexpr auto SurfaceAttributes = std::to_array<AttributeSpec>({
    PhysicalLengthAttribute("width", ParseWidthAttribute, true),
    PhysicalLengthAttribute("height", ParseHeightAttribute, true),
    PhysicalLengthAttribute("x-offset", ParseXOffsetAttribute),
    PhysicalLengthAttribute("y-offset", ParseYOffsetAttribute),
    PhysicalLengthAttribute("z-offset", ParseZOffsetAttribute),
    PhysicalAngleAttribute("x-rotation", ParseXRotationAttribute),
    PhysicalAngleAttribute("y-rotation", ParseYRotationAttribute),
    PhysicalAngleAttribute("z-rotation", ParseZRotationAttribute),
    StringAttribute("anchor"),
    StringAttribute("class"),
    StringAttribute("behavior"),
});
constexpr auto GroupAttributes = std::to_array<AttributeSpec>(
    {StringAttribute("class"), StringAttribute("behavior"),
     StringAttribute("name")});
constexpr auto TextAttributes = std::to_array<AttributeSpec>(
    {StringAttribute("class"), StringAttribute("behavior"),
     StateAttribute("state")});
constexpr auto ImageAttributes = std::to_array<AttributeSpec>(
    {StringAttribute("class"), StringAttribute("behavior"),
     StringAttribute("src"), StringAttribute("alt")});
constexpr auto PanelAttributes = std::to_array<AttributeSpec>(
    {StringAttribute("class"), StringAttribute("behavior"),
     StringAttribute("name")});
constexpr auto ScriptAttributes = std::to_array<AttributeSpec>(
    {AttributeSpec{"type", false, ParseScriptTypeAttribute}});
constexpr std::array<AttributeSpec, 0> NoAttributes{};
constexpr auto NodeSpecs = std::to_array<ElementSpec>({
    {"group", LNodeType::Group, GroupAttributes},
    {"row", LNodeType::Row, CommonAttributes},
    {"column", LNodeType::Column, CommonAttributes},
    {"stack", LNodeType::Stack, CommonAttributes},
    {"text", LNodeType::Text, TextAttributes},
    {"image", LNodeType::Image, ImageAttributes},
    {"panel", LNodeType::Panel, PanelAttributes},
});
constexpr ElementSpec SurfaceSpec{"surface", LNodeType::Surface,
                                  SurfaceAttributes};

const ElementSpec *FindNodeSpec(std::string_view name) {
  for (const auto &spec : NodeSpecs)
    if (spec.name == name)
      return &spec;
  return nullptr;
}

template <typename T>
bool ParseAttributes(ParseResult<Document> &result, std::string_view input,
                     pugi::xml_node element,
                     std::span<const AttributeSpec> specs, T &target,
                     std::string_view noAttributesMessage = {}) {
  for (auto attribute : element.attributes()) {
    const std::string_view name = attribute.name();
    const AttributeSpec *matched = nullptr;
    for (const auto &spec : specs)
      if (spec.name == name) {
        matched = &spec;
        break;
      }
    if (!matched) {
      if (!noAttributesMessage.empty())
        Error(result, input, element, std::string(noAttributesMessage));
      else
        Error(result, input, element,
              "invalid attribute '" + std::string(name) + "' on <" +
                  element.name() + "> (value '" + attribute.value() + "')");
      return false;
    }
    std::string reason;
    if (!matched->parser(&target, name, attribute.value(), reason)) {
      Error(result, input, element,
            "invalid <" + std::string(element.name()) + "> " +
                std::string(name) + ": " + reason);
      return false;
    }
  }
  for (const auto &spec : specs)
    if (spec.required && !element.attribute(spec.name.data())) {
      Error(result, input, element,
            "<" + std::string(element.name()) + "> is missing required '" +
                std::string(spec.name) + "' attribute");
      return false;
    }
  return true;
}

bool ReadNode(ParseResult<Document> &parsed, std::string_view input,
              pugi::xml_node source, LNode &result) {
  const auto *spec = FindNodeSpec(source.name());
  if (!spec) {
    Error(parsed, input, source,
          "unknown ARUI node <" + std::string(source.name()) + ">");
    return false;
  }
  result.type = spec->type;
  if (!ParseAttributes(parsed, input, source, spec->attributes, result))
    return false;
  std::string text;
  for (auto child : source.children()) {
    if (child.type() == pugi::node_element) {
      if (spec->type == LNodeType::Text || spec->type == LNodeType::Image) {
        Error(parsed, input, child,
              "<" + std::string(source.name()) +
                  "> cannot contain child elements");
        return false;
      }
      LNode node{};
      if (!ReadNode(parsed, input, child, node))
        return false;
      result.children.push_back(std::move(node));
    } else if (child.type() == pugi::node_pcdata ||
               child.type() == pugi::node_cdata) {
      text += child.value();
    }
  }
  text = Trim(text);
  if (spec->type == LNodeType::Text) {
    if (!text.empty())
      result.attributes.insert_or_assign("text", std::move(text));
  } else if (!text.empty()) {
    Error(parsed, input, source,
          "unexpected text content in <" + std::string(source.name()) + ">");
    return false;
  }
  return true;
}

std::string Trim(std::string_view value) {
  auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos)
    return {};
  auto last = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(first, last - first + 1));
}
class StyleParser {
public:
  explicit StyleParser(std::string_view input) : input(input) {}

  ParseResult<StyleSheet> Parse() {
    ParseResult<StyleSheet> result;
    while (SkipSpaceAndComments(result)) {
      if (position == input.size())
        break;
      auto selectorStart = position;
      while (position < input.size() && input[position] != '{')
        ++position;
      if (position == input.size()) {
        Error(result, selectorStart, "expected '{' after selector");
        break;
      }
      StyleRule rule;
      rule.selector =
          Trim(input.substr(selectorStart, position++ - selectorStart));
      if (rule.selector.empty()) {
        Error(result, selectorStart, "empty selector");
        break;
      }
      bool closed = false;
      while (SkipSpaceAndComments(result)) {
        if (position == input.size())
          break;
        if (input[position] == '}') {
          ++position;
          closed = true;
          break;
        }
        auto propertyStart = position;
        while (position < input.size() && input[position] != ':' &&
               input[position] != '}' && input[position] != ';')
          ++position;
        if (position == input.size() || input[position] != ':') {
          Error(result, propertyStart, "expected ':' after property");
          break;
        }
        Declaration declaration;
        declaration.property =
            Trim(input.substr(propertyStart, position++ - propertyStart));
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
            if (escaped)
              escaped = false;
            else if (c == '\\')
              escaped = true;
            else if (c == quote)
              quote = 0;
          } else if (c == '"' || c == '\'')
            quote = c;
          else if (c == '{')
            ++braces;
          else if (c == '(')
            ++parens;
          else if (c == ')') {
            if (parens == 0) {
              Error(result, position, "unmatched ')'");
              break;
            }
            --parens;
          } else if (c == '}') {
            if (braces == 0 && parens == 0)
              break;
            if (braces > 0)
              --braces;
          } else if (c == ';' && braces == 0 && parens == 0)
            break;
        }
        if (!result)
          break;
        if (quote || braces || parens) {
          Error(result, valueStart, "unterminated style value");
          break;
        }
        declaration.value =
            Trim(input.substr(valueStart, position - valueStart));
        if (declaration.value.empty()) {
          Error(result, valueStart, "empty value");
          break;
        }
        rule.declarations.push_back(std::move(declaration));
        if (position < input.size() && input[position] == ';')
          ++position;
      }
      if (!result)
        break;
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
      if (std::isspace(static_cast<unsigned char>(input[position]))) {
        ++position;
        continue;
      }
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
  void Error(ParseResult<StyleSheet> &result, std::size_t offset,
             std::string message) {
    Diagnostic diagnostic;
    for (std::size_t i = 0; i < offset && i < input.size(); ++i) {
      if (input[i] == '\n') {
        ++diagnostic.line;
        diagnostic.column = 1;
      } else
        ++diagnostic.column;
    }
    diagnostic.message = std::move(message);
    result.diagnostics.push_back(std::move(diagnostic));
  }
  std::string_view input;
  std::size_t position = 0;
};
} // namespace

std::optional<Length> ParseStyleLength(std::string_view input) {
  std::string reason;
  return ParsePhysicalLength(input, reason);
}
bool HasClass(const LNode &node, std::string_view wanted) {
  const auto *classes = node.GetAttribute<std::string>("class");
  if (!classes)
    return false;
  std::istringstream words(*classes);
  for (std::string word; words >> word;)
    if (word == wanted)
      return true;
  return false;
}
bool ApplyDeclaration(LNode &node, const Declaration &declaration) {
  if (declaration.property == "fill" || declaration.property == "stroke") {
    if (declaration.value == "none") {
      node.style.painterProperties[declaration.property] = declaration.value;
      return true;
    }
    const auto color = ParseSRGBHexColor(declaration.value);
    if (!color)
      return false;
    node.style.painterProperties[declaration.property] = *color;
    return true;
  }
  if (declaration.property == "painter") {
    node.style.painter = PainterReference{declaration.value};
    return true;
  }
  if (declaration.property == "padding" || declaration.property == "gap" ||
      declaration.property == "font-size" ||
      declaration.property == "stroke-width") {
    const auto length = ParseStyleLength(declaration.value);
    if (!length)
      return false;
    if (declaration.property == "padding")
      node.style.padding = *length;
    else if (declaration.property == "gap")
      node.style.gap = *length;
    else if (declaration.property == "font-size")
      node.style.fontSize = *length;
    else
      node.style.painterProperties[declaration.property] = *length;
  }
  return true;
}
bool ApplyRules(LNode &node, const std::vector<StyleRule> &rules) {
  for (const auto &rule : rules) {
    if (rule.selector.size() < 2 || rule.selector.front() != '.' ||
        !HasClass(node, std::string_view(rule.selector).substr(1)))
      continue;
    for (const auto &declaration : rule.declarations)
      if (!ApplyDeclaration(node, declaration))
        return false;
  }
  for (auto &child : node.children)
    if (!ApplyRules(child, rules))
      return false;
  return true;
}

ParseResult<Document> ParseMarkup(std::string_view source) {

  ParseResult<Document> result;
  pugi::xml_document xml;
  const auto status = xml.load_buffer(source.data(), source.size());
  if (!status) {
    Diagnostic diagnostic;
    for (std::ptrdiff_t i = 0;
         i < status.offset && i < static_cast<std::ptrdiff_t>(source.size());
         ++i) {
      if (source[i] == '\n') {
        ++diagnostic.line;
        diagnostic.column = 1;
      } else
        ++diagnostic.column;
    }
    diagnostic.message =
        std::string("XML syntax error: ") + status.description();
    result.diagnostics.push_back(std::move(diagnostic));
    return result;
  }

  const auto root = xml.document_element();
  std::size_t rootElements = 0;
  for (auto child : xml.children())
    if (child.type() == pugi::node_element)
      ++rootElements;
  if (!root || rootElements != 1 || std::string_view(root.name()) != "arui") {
    result.diagnostics.push_back(
        {.message = "ARUI document root must be exactly one <arui> element"});
    return result;
  }
  if (!ParseAttributes(result, source, root, NoAttributes, result.value,
                       "<arui> does not accept attributes"))
    return result;

  for (auto child : root.children()) {
    if (child.type() == pugi::node_comment ||
        child.type() == pugi::node_declaration)
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
      if (!ParseAttributes(result, source, child, NoAttributes, result.value,
                           "<style> does not accept attributes"))
        return result;
      std::string stylesheet;
      for (auto content : child.children()) {
        if (content.type() == pugi::node_pcdata ||
            content.type() == pugi::node_cdata)
          stylesheet += content.value();
        else if (content.type() == pugi::node_element) {
          Error(result, source, content,
                "<style> cannot contain markup elements");
          return result;
        }
      }
      result.value.stylesheets.push_back(std::move(stylesheet));
    } else if (name == "script") {
      if (const auto external = child.attribute("src")) {
        Error(result, source, child,
              "External ARUI scripts are not supported yet: " +
                  std::string(external.value()));
        return result;
      }
      EmbeddedScript script;
      if (!ParseAttributes(result, source, child, ScriptAttributes, script))
        return result;
      for (auto content : child.children()) {
        if (content.type() == pugi::node_pcdata ||
            content.type() == pugi::node_cdata)
          script.source += content.value();
        else if (content.type() == pugi::node_element) {
          Error(result, source, content,
                "<script> cannot contain markup elements");
          return result;
        }
      }
      result.value.scripts.push_back(std::move(script));
    } else if (name == "surface") {
      LNode surface{.type = SurfaceSpec.type};
      if (!ParseAttributes(result, source, child, SurfaceSpec.attributes,
                           surface))
        return result;
      std::string surfaceText;
      for (auto content : child.children()) {
        if (content.type() == pugi::node_element) {
          LNode node{};
          if (!ReadNode(result, source, content, node))
            return result;
          surface.children.push_back(std::move(node));
        } else if (content.type() == pugi::node_pcdata ||
                   content.type() == pugi::node_cdata) {
          surfaceText += content.value();
        }
      }
      if (!Trim(surfaceText).empty()) {
        Error(result, source, child, "unexpected text content in <surface>");
        return result;
      }
      result.value.surfaces.push_back(std::move(surface));
    } else {
      Error(result, source, child,
            "unsupported top-level ARUI element <" + std::string(child.name()) +
                ">");
      return result;
    }
  }
  std::vector<StyleRule> rules;
  for (const auto &sourceSheet : result.value.stylesheets) {
    auto sheet = ParseStyles(sourceSheet);
    if (!sheet) {
      result.diagnostics.insert(result.diagnostics.end(),
                                sheet.diagnostics.begin(),
                                sheet.diagnostics.end());
      return result;
    }
    rules.insert(rules.end(), sheet.value.rules.begin(),
                 sheet.value.rules.end());
  }
  for (auto &surface : result.value.surfaces) {
    if (!ApplyRules(surface, rules)) {
      result.diagnostics.push_back(
          {.message = "invalid typed style value (colors must be #RRGGBB or "
                      "#RRGGBBAA)"});
      return result;
    }
  }
  return result;
}

ParseResult<Document> ParseMarkupFile(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
    return {.diagnostics = {
                {.message = "unable to read ARUI file: " + path.string()}}};
  std::ostringstream contents;
  contents << stream.rdbuf();
  return ParseMarkup(contents.str());
}

ParseResult<StyleSheet> ParseStyles(std::string_view source) {
  return StyleParser(source).Parse();
}
} // namespace ARUI::Language
