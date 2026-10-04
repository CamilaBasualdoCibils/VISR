#pragma once

#include "ARUI/Presentation/IPresentationController.hpp"

#include <memory>
#include <string>

namespace ARUI::Presentation {

inline constexpr std::uint16_t DefaultPresentationPort = 4243;

class RpcPresentationController final : public IPresentationController {
public:
  explicit RpcPresentationController(
      std::string host = "127.0.0.1",
      std::uint16_t port = DefaultPresentationPort);
  ~RpcPresentationController() override;
  RpcPresentationController(RpcPresentationController &&) noexcept;
  RpcPresentationController &operator=(RpcPresentationController &&) noexcept;

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
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace ARUI::Presentation
