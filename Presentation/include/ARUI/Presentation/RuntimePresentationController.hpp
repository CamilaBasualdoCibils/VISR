#pragma once

#include "ARUI/Presentation/IPresentationController.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"

#include <mutex>

namespace ARUI::Presentation {

class RuntimePresentationController final : public IPresentationController {
public:
  RuntimePresentationController(Runtime::RuntimeTree &runtime,
                                std::mutex &runtimeMutex);

  PresentationNodeID CreateNode(PresentationNodeID parent,
                                const Language::LNode &node,
                                Transition transition = {}) override;
  PresentationNodeID CreateTree(PresentationNodeID parent,
                                const Language::LNode &tree,
                                Transition transition = {}) override;
  void Remove(PresentationNodeID node, Transition transition = {}) override;
  void Move(PresentationNodeID node, PresentationNodeID newParent,
            Transition transition = {}) override;
  void SetStyle(PresentationNodeID node, Language::Style style,
                Transition transition = {}) override;
  void SetVisibility(PresentationNodeID node, bool visible,
                     Transition transition = {}) override;
  void ReorderChildren(PresentationNodeID parent,
                       std::span<const PresentationNodeID> children,
                       Transition transition = {}) override;
  void Reset(Transition transition = {}) override;

private:
  Runtime::RuntimeTree &runtime_;
  std::mutex &runtimeMutex_;
};

} // namespace ARUI::Presentation
