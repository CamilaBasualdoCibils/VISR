#include "Wire.hpp"

#include "VISR/Language/Color.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>

namespace VISR::Presentation::Wire {
namespace {
using Json = nlohmann::json;
using namespace VISR::Language;

Json EncodeLength(const Length &value) {
  return {{"value", value.value}, {"unit", static_cast<int>(value.unit)}};
}
Length DecodeLength(const Json &value) {
  return {value.at("value").get<double>(),
          static_cast<LengthUnit>(value.at("unit").get<int>())};
}
Json EncodeAngle(const Angle &value) {
  return {{"value", value.value}, {"unit", static_cast<int>(value.unit)}};
}
Angle DecodeAngle(const Json &value) {
  return {value.at("value").get<double>(),
          static_cast<AngleUnit>(value.at("unit").get<int>())};
}
Json EncodeColor(const Color &value) {
  return Json::array({value.rgba.r, value.rgba.g, value.rgba.b, value.rgba.a});
}
Color DecodeColor(const Json &value) {
  return {{value.at(0).get<float>(), value.at(1).get<float>(),
           value.at(2).get<float>(), value.at(3).get<float>()}};
}

template <typename T, typename Encode>
Json EncodeOptional(const std::optional<T> &value, Encode encode) {
  return value ? encode(*value) : Json(nullptr);
}
template <typename T, typename Decode>
std::optional<T> DecodeOptional(const Json &value, Decode decode) {
  return value.is_null() ? std::nullopt : std::optional<T>{decode(value)};
}

Json EncodeAttribute(const AttributeValue &attribute) {
  Json encoded{{"type", attribute.index()}};
  std::visit(
      [&](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Length>)
          encoded["value"] = EncodeLength(value);
        else if constexpr (std::is_same_v<T, Angle>)
          encoded["value"] = EncodeAngle(value);
        else if constexpr (std::is_same_v<T, StateReference> ||
                           std::is_same_v<T, ActionReference>)
          encoded["value"] = value.value;
        else
          encoded["value"] = value;
      },
      attribute);
  return encoded;
}
AttributeValue DecodeAttribute(const Json &attribute) {
  const auto &value = attribute.at("value");
  switch (attribute.at("type").get<std::size_t>()) {
  case 0:
    return value.get<bool>();
  case 1:
    return value.get<std::int64_t>();
  case 2:
    return value.get<double>();
  case 3:
    return value.get<std::string>();
  case 4:
    return DecodeLength(value);
  case 5:
    return DecodeAngle(value);
  case 6:
    return StateReference{value.get<std::string>()};
  case 7:
    return ActionReference{value.get<std::string>()};
  default:
    throw std::invalid_argument("unknown VISR attribute wire type");
  }
}

Json EncodePainterValue(const PainterStyleValue &property) {
  Json encoded{{"type", property.index()}};
  std::visit(
      [&](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Length>)
          encoded["value"] = EncodeLength(value);
        else if constexpr (std::is_same_v<T, Angle>)
          encoded["value"] = EncodeAngle(value);
        else if constexpr (std::is_same_v<T, Color>)
          encoded["value"] = EncodeColor(value);
        else
          encoded["value"] = value;
      },
      property);
  return encoded;
}
PainterStyleValue DecodePainterValue(const Json &property) {
  const auto &value = property.at("value");
  switch (property.at("type").get<std::size_t>()) {
  case 0:
    return value.get<bool>();
  case 1:
    return value.get<std::int64_t>();
  case 2:
    return value.get<double>();
  case 3:
    return value.get<std::string>();
  case 4:
    return DecodeLength(value);
  case 5:
    return DecodeAngle(value);
  case 6:
    return DecodeColor(value);
  default:
    throw std::invalid_argument("unknown VISR painter-property wire type");
  }
}

Json EncodeStyleJson(const Style &style) {
  Json properties = Json::object();
  for (const auto &[name, value] : style.painterProperties)
    properties[name] = EncodePainterValue(value);
  return {
      {"width", EncodeLength(style.width)},
      {"height", EncodeLength(style.height)},
      {"minWidth", EncodeLength(style.minWidth)},
      {"minHeight", EncodeLength(style.minHeight)},
      {"maxWidth", EncodeLength(style.maxWidth)},
      {"maxHeight", EncodeLength(style.maxHeight)},
      {"margin", EncodeOptional(style.margin, EncodeLength)},
      {"padding", EncodeOptional(style.padding, EncodeLength)},
      {"gap", EncodeOptional(style.gap, EncodeLength)},
      {"fontSize", EncodeOptional(style.fontSize, EncodeLength)},
      {"xOffset", EncodeOptional(style.xOffset, EncodeLength)},
      {"yOffset", EncodeOptional(style.yOffset, EncodeLength)},
      {"zOffset", EncodeOptional(style.zOffset, EncodeLength)},
      {"xRotation", EncodeOptional(style.xRotation, EncodeAngle)},
      {"yRotation", EncodeOptional(style.yRotation, EncodeAngle)},
      {"zRotation", EncodeOptional(style.zRotation, EncodeAngle)},
      {"shape", style.shape ? Json(*style.shape) : Json(nullptr)},
      {"radius", EncodeOptional(style.radius, EncodeLength)},
      {"arc", EncodeOptional(style.arc, EncodeAngle)},
      {"painter", style.painter ? Json(style.painter->name) : Json(nullptr)},
      {"painterProperties", std::move(properties)},
  };
}
Style DecodeStyleJson(const Json &encoded) {
  Style style;
  style.width = DecodeLength(encoded.at("width"));
  style.height = DecodeLength(encoded.at("height"));
  style.minWidth = DecodeLength(encoded.at("minWidth"));
  style.minHeight = DecodeLength(encoded.at("minHeight"));
  style.maxWidth = DecodeLength(encoded.at("maxWidth"));
  style.maxHeight = DecodeLength(encoded.at("maxHeight"));
  style.margin = DecodeOptional<Length>(encoded.at("margin"), DecodeLength);
  style.padding = DecodeOptional<Length>(encoded.at("padding"), DecodeLength);
  style.gap = DecodeOptional<Length>(encoded.at("gap"), DecodeLength);
  style.fontSize = DecodeOptional<Length>(encoded.at("fontSize"), DecodeLength);
  style.xOffset = DecodeOptional<Length>(encoded.at("xOffset"), DecodeLength);
  style.yOffset = DecodeOptional<Length>(encoded.at("yOffset"), DecodeLength);
  style.zOffset = DecodeOptional<Length>(encoded.at("zOffset"), DecodeLength);
  style.xRotation = DecodeOptional<Angle>(encoded.at("xRotation"), DecodeAngle);
  style.yRotation = DecodeOptional<Angle>(encoded.at("yRotation"), DecodeAngle);
  style.zRotation = DecodeOptional<Angle>(encoded.at("zRotation"), DecodeAngle);
  if (!encoded.at("shape").is_null())
    style.shape = encoded.at("shape").get<std::string>();
  style.radius = DecodeOptional<Length>(encoded.at("radius"), DecodeLength);
  style.arc = DecodeOptional<Angle>(encoded.at("arc"), DecodeAngle);
  if (!encoded.at("painter").is_null())
    style.painter = PainterReference{encoded.at("painter").get<std::string>()};
  for (const auto &[name, value] : encoded.at("painterProperties").items())
    style.painterProperties.emplace(name, DecodePainterValue(value));
  return style;
}

Json EncodeNodeJson(const LNode &node) {
  Json attributes = Json::object();
  for (const auto &[name, value] : node.attributes)
    attributes[name] = EncodeAttribute(value);
  Json children = Json::array();
  for (const auto &child : node.children)
    children.push_back(EncodeNodeJson(child));
  return {{"id", node.id},
          {"type", static_cast<int>(node.type)},
          {"surfaceType", static_cast<int>(node.surfaceType)},
          {"attributes", std::move(attributes)},
          {"style", EncodeStyleJson(node.style)},
          {"children", std::move(children)}};
}
LNode DecodeNodeJson(const Json &encoded) {
  LNode node{.id = encoded.at("id").get<LNodeID>(),
             .type = static_cast<LNodeType>(encoded.at("type").get<int>()),
             .surfaceType =
                 static_cast<SurfaceType>(encoded.at("surfaceType").get<int>()),
             .style = DecodeStyleJson(encoded.at("style"))};
  for (const auto &[name, value] : encoded.at("attributes").items())
    node.attributes.emplace(name, DecodeAttribute(value));
  for (const auto &child : encoded.at("children"))
    node.children.push_back(DecodeNodeJson(child));
  return node;
}
} // namespace

std::string EncodeNode(const Language::LNode &node) {
  return EncodeNodeJson(node).dump();
}
Language::LNode DecodeNode(std::string_view json) {
  return DecodeNodeJson(Json::parse(json));
}
std::string EncodeStyle(const Language::Style &style) {
  return EncodeStyleJson(style).dump();
}
Language::Style DecodeStyle(std::string_view json) {
  return DecodeStyleJson(Json::parse(json));
}
} // namespace VISR::Presentation::Wire
