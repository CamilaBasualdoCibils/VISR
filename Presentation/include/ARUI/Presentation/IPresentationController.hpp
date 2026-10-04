#pragma once

#include "ARUI/Language/node.hpp"

#include <chrono>
#include <cstdint>
#include <span>

namespace ARUI::Presentation {

using PresentationNodeID = std::uint64_t;
inline constexpr PresentationNodeID PresentationRoot = 0;

enum class Easing { Linear, EaseInOut };

struct Transition {
  std::chrono::milliseconds duration{};
  Easing easing{Easing::Linear};
};

// Privileged control surface for AR-OS system experiences. This interface is
// intentionally separate from Application::IApplication.
class IPresentationController {
public:
  virtual ~IPresentationController() = default;

  virtual PresentationNodeID CreateNode(PresentationNodeID parent,
                                        const Language::LNode &node,
                                        Transition transition = {}) = 0;
  virtual PresentationNodeID CreateTree(PresentationNodeID parent,
                                        const Language::LNode &tree,
                                        Transition transition = {}) = 0;
  virtual void Remove(PresentationNodeID node, Transition transition = {}) = 0;
  virtual void Move(PresentationNodeID node, PresentationNodeID newParent,
                    Transition transition = {}) = 0;
  virtual void SetStyle(PresentationNodeID node, Language::Style style,
                        Transition transition = {}) = 0;
  virtual void SetVisibility(PresentationNodeID node, bool visible,
                             Transition transition = {}) = 0;
  virtual void ReorderChildren(PresentationNodeID parent,
                               std::span<const PresentationNodeID> children,
                               Transition transition = {}) = 0;
  virtual void Reset(Transition transition = {}) = 0;
};

} // namespace ARUI::Presentation
