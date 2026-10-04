#pragma once

#include "VISR/Presentation/IPresentationController.hpp"

#include <memory>
#include <string>

namespace VISR::Presentation {

inline constexpr std::uint16_t DefaultPresentationPort = 4243;

class RpcPresentationController final : public IPresentationController {
public:
  explicit RpcPresentationController(
      std::string host = "127.0.0.1",
      std::uint16_t port = DefaultPresentationPort);
  ~RpcPresentationController() override;
  RpcPresentationController(RpcPresentationController &&) noexcept;
  RpcPresentationController &operator=(RpcPresentationController &&) noexcept;

  [[nodiscard]] Language::LNode GetActiveTree() const override;
  void SetActiveTree(Language::LNode tree) override;
  void AddTree(Language::LNodeID parent, Language::LNode tree) override;
  void UpdateNode(Language::LNodeID node, Language::LNode replacement) override;
  void Move(Language::LNodeID node, Language::LNodeID newParent) override;
  void Remove(Language::LNodeID node) override;
  void Clear() override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace VISR::Presentation
