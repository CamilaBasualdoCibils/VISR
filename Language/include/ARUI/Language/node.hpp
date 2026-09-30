
#pragma once

#include "ARUI/Language/AttributeValue.hpp"
#include "ARUI/Language/Style.hpp"
#include <boost/describe/class.hpp>
#include <boost/describe/members.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/detail/mp_list.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
namespace ARUI::Language {

using LNodeID = uint64_t;

enum class LNodeType {
  Surface,

  Group,

  Row,
  Column,
  Stack,

  Text,
  Button,

  Panel
};

struct LNode {
  LNodeID id{};
  LNodeType type;

  Attributes attributes;
  Style style;

  std::vector<LNode> children;

  template <typename T>
  [[nodiscard]]
  const T *GetAttribute(std::string_view name) const {
    auto it = attributes.find(std::string{name});

    if (it == attributes.end())
      return nullptr;

    return std::get_if<T>(&it->second);
  }

  template <typename T> void SetAttribute(std::string name, T &&value) {
    attributes.insert_or_assign(std::move(name),
                                AttributeValue{std::forward<T>(value)});
  }

  [[nodiscard]]
  bool HasAttribute(std::string_view name) const {
    return attributes.contains(std::string{name});
  }
};

// ============================================================
// Generic option -> attribute conversion
// ============================================================

template <typename Options> Attributes AttributesFrom(const Options &options) {
  Attributes attributes;

  using Members =
      boost::describe::describe_members<Options, boost::describe::mod_public>;

  boost::mp11::mp_for_each<Members>([&](auto descriptor) {
    const auto &option = options.*descriptor.pointer;

    if (option.has_value()) {
      attributes.insert_or_assign(descriptor.name, *option);
    }
  });

  return attributes;
}

// ============================================================
// Surface
// ============================================================

struct SurfaceOptions {
  std::optional<std::string> anchor;
};

BOOST_DESCRIBE_STRUCT(SurfaceOptions, (),
                      (anchor))

inline LNode LSurface(std::vector<LNode> children = {},
                      SurfaceOptions options = {}, Style style = {}) {
  return {
      .type = LNodeType::Surface,
      .attributes = AttributesFrom(options),
      .style = std::move(style),
      .children = std::move(children),
  };
}

// ============================================================
// Group
// ============================================================

struct GroupOptions {
  std::optional<std::string> name;
};

BOOST_DESCRIBE_STRUCT(GroupOptions, (), (name))

inline LNode LGroup(std::vector<LNode> children = {}, GroupOptions options = {},
                    Style style = {}) {
  return {
      .type = LNodeType::Group,
      .attributes = AttributesFrom(options),
      .style = std::move(style),
      .children = std::move(children),
  };
}

// ============================================================
// Row
// ============================================================

struct RowOptions {};

BOOST_DESCRIBE_STRUCT(RowOptions, (), ())

inline LNode LRow(std::vector<LNode> children = {}, RowOptions options = {},
                  Style style = {}) {
  return {
      .type = LNodeType::Row,
      .attributes = AttributesFrom(options),
      .style = std::move(style),
      .children = std::move(children),
  };
}

// ============================================================
// Column
// ============================================================

struct ColumnOptions {};

BOOST_DESCRIBE_STRUCT(ColumnOptions, (), ())

inline LNode LColumn(std::vector<LNode> children = {},
                     ColumnOptions options = {}, Style style = {}) {
  return {
      .type = LNodeType::Column,
      .attributes = AttributesFrom(options),
      .style = std::move(style),
      .children = std::move(children),
  };
}

// ============================================================
// Stack
// ============================================================

struct StackOptions {};

BOOST_DESCRIBE_STRUCT(StackOptions, (), ())

inline LNode LStack(std::vector<LNode> children = {}, StackOptions options = {},
                    Style style = {}) {
  return {
      .type = LNodeType::Stack,
      .attributes = AttributesFrom(options),
      .style = std::move(style),
      .children = std::move(children),
  };
}

// ============================================================
// Text
// ============================================================

struct TextOptions {
  std::optional<StateReference> state;
};

BOOST_DESCRIBE_STRUCT(TextOptions, (), (state))

inline LNode LText(std::string text, TextOptions options = {},
                   Style style = {}) {
  auto attributes = AttributesFrom(options);

  attributes.insert_or_assign("text", std::move(text));

  return {
      .type = LNodeType::Text,
      .attributes = std::move(attributes),
      .style = std::move(style),
  };
}

inline LNode LText(StateReference state, TextOptions options = {},
                   Style style = {}) {
  auto attributes = AttributesFrom(options);

  attributes.insert_or_assign("state", std::move(state));

  return {
      .type = LNodeType::Text,
      .attributes = std::move(attributes),
      .style = std::move(style),
  };
}

// ============================================================
// Button
// ============================================================

struct ButtonOptions {
  std::optional<bool> disabled;
};

BOOST_DESCRIBE_STRUCT(ButtonOptions, (), (disabled))

inline LNode LButton(ActionReference action, std::vector<LNode> children = {},
                     ButtonOptions options = {}, Style style = {}) {
  auto attributes = AttributesFrom(options);

  attributes.insert_or_assign("action", std::move(action));

  return {
      .type = LNodeType::Button,
      .attributes = std::move(attributes),
      .style = std::move(style),
      .children = std::move(children),
  };
}

// ============================================================
// Panel
// ============================================================

struct PanelOptions {
  std::optional<std::string> name;
  std::optional<std::string> script;
};

BOOST_DESCRIBE_STRUCT(PanelOptions, (), (name, script))

inline LNode LPanel(std::vector<LNode> children = {}, PanelOptions options = {},
                    Style style = {}) {
  return {
      .type = LNodeType::Panel,
      .attributes = AttributesFrom(options),
      .style = std::move(style),
      .children = std::move(children),
  };
}
} // namespace ARUI::Language
