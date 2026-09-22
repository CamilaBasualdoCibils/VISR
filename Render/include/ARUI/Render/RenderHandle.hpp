#pragma once

#include <functional>
namespace ARUI::Render
{
    template <typename Tag, typename T = int> struct Handle {
  T value{};
    
  using Type = T;
  constexpr Handle() = default;
  constexpr Handle(T v) : value(v) {}
  constexpr Handle(const Handle &) = default;
  constexpr Handle(Handle &&) = default;

  constexpr Handle &operator=(const Handle &) = default;
  // Explicit cast back to underlying integer
  constexpr explicit operator T() const { return value; }

  // Comparison
  constexpr auto operator<=>(const Handle &) const = default;

  // Bitwise
  constexpr Handle operator|(Handle rhs) const { return value | rhs.value; }
  constexpr Handle operator&(Handle rhs) const { return value & rhs.value; }
  constexpr Handle operator^(Handle rhs) const { return value ^ rhs.value; }
  constexpr Handle operator~() const { return ~value; }

  constexpr Handle &operator|=(Handle rhs) {
    value |= rhs.value;
    return *this;
  }
  constexpr Handle &operator&=(Handle rhs) {
    value &= rhs.value;
    return *this;
  }
  constexpr Handle &operator^=(Handle rhs) {
    value ^= rhs.value;
    return *this;
  }
  constexpr Handle &operator++() {
    ++value;
    return *this;
  }
  constexpr Handle operator++(int) {
    Handle temp = *this;
    ++value;
    return temp;
  }
  constexpr Handle &operator--() {
    --value;
    return *this;
  }
  constexpr Handle operator--(int) {
    Handle temp = *this;
    --value;
    return temp;
  }
};
}
namespace std {
    template <typename Tag, typename T>
    struct hash<ARUI::Render::Handle<Tag, T>> {
        std::size_t operator()(const ARUI::Render::Handle<Tag, T>& handle) const noexcept {
            return std::hash<T>{}(static_cast<T>(handle));
        }
    };
}