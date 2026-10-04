#pragma once

#include "VISR/Runtime/IPainter.hpp"

namespace VISR::Runtime {

class VisrFlatPainter final : public IPainter {
public:
  void Paint(const PaintTreeContext &context,
             const Language::PainterStyleProperties &properties) const override;

  [[nodiscard]] const Language::PainterStyleSchema &
  StyleSchema() const noexcept override;
};

void RegisterVisrFlatPainter(PainterRegistry &registry);

} // namespace VISR::Runtime
