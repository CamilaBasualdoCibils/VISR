#pragma once

#include "ARUI/Language/node.hpp"

namespace ARUI::Presentation {

// Privileged, immediate control of the active ARUI language tree. Animation
// belongs to the action system, not to these structural mutations.
class IPresentationController {
public:
  virtual ~IPresentationController() = default;

  [[nodiscard]] virtual Language::LNode GetActiveTree() const = 0;
  virtual void SetActiveTree(Language::LNode tree) = 0;
  virtual void AddTree(Language::LNodeID parent, Language::LNode tree) = 0;
  virtual void UpdateNode(Language::LNodeID node,
                          Language::LNode replacement) = 0;
  virtual void Move(Language::LNodeID node,
                    Language::LNodeID newParent) = 0;
  virtual void Remove(Language::LNodeID node) = 0;
  virtual void Clear() = 0;
};

} // namespace ARUI::Presentation
