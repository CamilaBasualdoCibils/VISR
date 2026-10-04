#pragma once

#include "VISR/Language/Style.hpp"
#include "VISR/Runtime/RuntimeTree.hpp"

#include <glm/ext/vector_float2.hpp>
#include <string>
#include <vector>

namespace VISR::Runtime::PainterCommons {

struct LayoutDefaults {
  Language::Length padding{0.0, Language::LengthUnit::Meter};
  Language::Length gap{0.0, Language::LengthUnit::Meter};
  Language::Length fontSize{5.0, Language::LengthUnit::Millimeter};
  std::string fontFamily{"Noto Sans"};
};

// Coordinates are physical surface-local coordinates measured in meters.
// Camera and presentation transforms are outside this layout library.
struct LayoutBounds {
  glm::vec2 minimum{};
  glm::vec2 maximum{};

  [[nodiscard]] glm::vec2 Size() const noexcept { return maximum - minimum; }
};

struct LayoutNode {
  NodeID id{};
  LayoutBounds bounds;
  LayoutBounds contentBounds;
  Language::Length fontSize;
  std::string fontFamily;
  std::vector<LayoutNode> children;
};

[[nodiscard]] LayoutNode LayoutTree(const RuntimeTree &tree, NodeID root,
                                    const LayoutBounds &rootBounds,
                                    const LayoutDefaults &defaults);

} // namespace VISR::Runtime::PainterCommons
