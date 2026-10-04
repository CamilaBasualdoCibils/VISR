#include "VISR/Runtime/IPainter.hpp"

#include <stdexcept>
#include <utility>

namespace VISR::Runtime {

void PainterRegistry::Register(std::string name,
                               std::shared_ptr<const IPainter> painter) {
  if (name.empty())
    throw std::invalid_argument("painter name must not be empty");
  if (!painter)
    throw std::invalid_argument("painter must not be null");
  if (!painters_.emplace(std::move(name), std::move(painter)).second)
    throw std::logic_error("a painter with that name is already registered");
}

const IPainter *PainterRegistry::Find(std::string_view name) const noexcept {
  const auto found = painters_.find(std::string{name});
  return found == painters_.end() ? nullptr : found->second.get();
}

} // namespace VISR::Runtime
