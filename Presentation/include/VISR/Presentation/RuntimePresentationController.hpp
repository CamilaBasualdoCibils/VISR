#pragma once

#include "VISR/Presentation/IPresentationController.hpp"
#include "VISR/Runtime/RuntimeTree.hpp"

#include <mutex>

namespace VISR::Presentation {

class RuntimePresentationController final : public IPresentationController {
public:
  RuntimePresentationController(Runtime::RuntimeTree &runtime,
                                std::mutex &runtimeMutex);

  [[nodiscard]] Language::LNode GetActiveTree() const override;
  void SetActiveTree(Language::LNode tree) override;
  void AddTree(Language::LNodeID parent, Language::LNode tree) override;
  void UpdateNode(Language::LNodeID node, Language::LNode replacement) override;
  void Move(Language::LNodeID node, Language::LNodeID newParent) override;
  void Remove(Language::LNodeID node) override;
  void Clear() override;

private:
  Runtime::RuntimeTree &runtime_;
  std::mutex &runtimeMutex_;
};

} // namespace VISR::Presentation
