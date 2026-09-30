#pragma once

#include "ARUI/Core/Application/Value.hpp"
#include "ARUI/Language/node.hpp"
#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace ARUI::Application {

using StateID = uint64_t;
using ActionID = uint64_t;

struct StateDescriptor {
  StateID id;
  std::string name;
  ValueType type;

  std::optional<std::string> description;
};
struct StateChange {
  StateID state;
  Value value;
};

struct ParameterDescriptor {
  std::string name;
  ValueType type;
  bool required = true;
  std::optional<std::string> description;
};

struct ActionDescriptor {
  ActionID id;
  std::string name;
  std::vector<ParameterDescriptor> parameters;
  std::optional<std::string> description;
};
enum class ActionErrorCode { Unknown, InvalidParameter, ExecutionFailed };
struct ActionError {
  ActionErrorCode code;
  std::string message;
};
using ActionResult = std::expected<std::optional<Value>, ActionError>;
using TemplateID = uint64_t;

struct TemplateDescriptor {
  TemplateID id;

  std::string name;
  std::string description;

  Language::LNode root;
};


class IApplication {
public:
  virtual std::span<const StateDescriptor> States() const = 0;
  virtual std::span<const ActionDescriptor> Actions() const = 0;

  virtual Value GetState(StateID) = 0;
  virtual ActionResult Invoke(ActionID,
                              std::span<const Value> arguments = {}) = 0;

  virtual std::span<const TemplateDescriptor> Templates() const = 0;
};

} // namespace ARUI::Application
