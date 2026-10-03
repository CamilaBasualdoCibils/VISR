#include "ARUI/Language/Serializer.hpp"
#include <pugixml.hpp>
#include <type_traits>

namespace ARUI::Language {
namespace {
const char *NodeName(LNodeType type) {
  switch (type) {
  case LNodeType::Surface:
    return "surface";
  case LNodeType::Group:
    return "group";
  case LNodeType::Row:
    return "row";
  case LNodeType::Column:
    return "column";
  case LNodeType::Stack:
    return "stack";
  case LNodeType::Text:
    return "text";
  case LNodeType::Image:
    return "image";
  case LNodeType::Button:
    return "button";
  case LNodeType::Panel:
    return "panel";
  }
  return "group";
}

std::string AttributeText(const AttributeValue &value) {
  return std::visit(
      []<typename T>(const T &v) -> std::string {
        if constexpr (std::is_same_v<T, bool>)
          return v ? "true" : "false";
        else if constexpr (std::is_same_v<T, int64_t> ||
                           std::is_same_v<T, double>)
          return std::to_string(v);
        else if constexpr (std::is_same_v<T, std::string>)
          return v;
        else if constexpr (std::is_same_v<T, StateReference> ||
                           std::is_same_v<T, ActionReference>)
          return v.value;
        else {
          std::string unit;
          if constexpr (std::is_same_v<T, Length>) {
            switch (v.unit) {
            case LengthUnit::Millimeter:
              unit = "mm";
              break;
            case LengthUnit::Centimeter:
              unit = "cm";
              break;
            case LengthUnit::Meter:
              unit = "m";
              break;
            case LengthUnit::Percent:
              unit = "%";
              break;
            case LengthUnit::Auto:
              unit = "";
              break;
            }
          }
          return std::to_string(v.value) + unit;
        }
      },
      value);
}

void WriteNode(pugi::xml_node parent, const LNode &node) {
  auto element = parent.append_child(NodeName(node.type));
  for (const auto &[name, value] : node.attributes) {
    if (node.type == LNodeType::Text && name == "text") {
      element.text().set(AttributeText(value).c_str());
      continue;
    }
    element.append_attribute(name.c_str())
        .set_value(AttributeText(value).c_str());
  }
  if (node.type == LNodeType::Surface) {
    element.append_attribute("width").set_value(
        AttributeText(node.style.width).c_str());
    element.append_attribute("height").set_value(
        AttributeText(node.style.height).c_str());
  }
  for (const auto &child : node.children)
    WriteNode(element, child);
}
} // namespace

std::string SerializeMarkup(const Document &document) {
  pugi::xml_document xml;
  auto root = xml.append_child("arui");
  for (const auto &stylesheet : document.stylesheets)
    root.append_child("style")
        .append_child(pugi::node_cdata)
        .set_value(stylesheet.c_str());
  for (const auto &script : document.scripts) {
    auto element = root.append_child("script");
    if (!script.type.empty())
      element.append_attribute("type").set_value(script.type.c_str());
    element.append_child(pugi::node_cdata).set_value(script.source.c_str());
  }
  for (const auto &surface : document.surfaces)
    WriteNode(root, surface);
  std::string output;
  struct Writer : pugi::xml_writer {
    std::string &output;
    explicit Writer(std::string &value) : output(value) {}
    void write(const void *data, size_t size) override {
      output.append(static_cast<const char *>(data), size);
    }
  } writer(output);
  xml.save(writer, "  ", pugi::format_no_declaration);
  return output;
}

std::string SerializeStyles(const StyleSheet &sheet) {
  std::string output;
  for (const auto &rule : sheet.rules) {
    output += rule.selector + " {\n";
    for (const auto &declaration : rule.declarations)
      output += "  " + declaration.property + ": " + declaration.value + ";\n";
    output += "}\n";
  }
  return output;
}
} // namespace ARUI::Language
