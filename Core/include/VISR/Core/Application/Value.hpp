#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
namespace VISR::Application {

enum class ValueType {
  Null,
  Boolean,
  Integer,
  UnsignedInteger,
  Double,
  String,
  Array,
  Object
};
struct Value;

using Array = std::vector<Value>;
using Object = std::unordered_map<std::string, Value>;

struct Value {
  using Storage = std::variant<std::monostate, bool, int64_t, uint64_t, double,
                               std::string, Array, Object>;

  Storage data;
  ValueType type() const { return static_cast<ValueType>(data.index()); }
};


} // namespace VISR::Application