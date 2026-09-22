#include "ARUI/Language/Serializer.hpp"
#include <pugixml.hpp>

namespace ARUI::Language {
namespace {
void WriteNode(pugi::xml_node parent, const LNode &node) {
  if (node.name.empty()) {
    parent.append_child(pugi::node_pcdata).set_value(node.text.c_str());
    return;
  }
  auto element = parent.append_child(node.name.c_str());
  for (const auto &attribute : node.attributes)
    element.append_attribute(attribute.name.c_str()).set_value(attribute.value.c_str());
  for (const auto &child : node.children) WriteNode(element, child);
}
} // namespace

std::string SerializeMarkup(const Document &document) {
  pugi::xml_document xml;
  WriteNode(xml, document.root);
  std::string output;
  struct Writer : pugi::xml_writer {
    std::string &output;
    explicit Writer(std::string &value) : output(value) {}
    void write(const void *data, size_t size) override {
      output.append(static_cast<const char *>(data), size);
    }
  } writer(output);
  xml.save(writer, "  ", pugi::format_no_declaration | pugi::format_raw);
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
