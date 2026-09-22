#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace ARUI::Language {
struct Diagnostic {
  std::size_t line = 1, column = 1;
  std::string message;
};
template <typename T> struct ParseResult {
  T value{};
  std::vector<Diagnostic> diagnostics;
  explicit operator bool() const { return diagnostics.empty(); }
};
} // namespace ARUI::Language
