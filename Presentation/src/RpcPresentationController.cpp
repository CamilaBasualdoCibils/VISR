#include "ARUI/Presentation/RpcPresentationController.hpp"

#include "Wire.hpp"

#include <chrono>
#include <rpc/client.h>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ARUI::Presentation {
class RpcPresentationController::Impl {
public:
  Impl(std::string host, std::uint16_t port) : client(std::move(host), port) {
    for (int attempt = 0; attempt < 100; ++attempt) {
      try {
        if (client.call("presentation.ping").as<bool>())
          return;
      } catch (const std::exception &) {
        if (attempt == 99)
          throw;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    throw std::runtime_error("ARUI presentation service is unavailable");
  }
  rpc::client client;
};

RpcPresentationController::RpcPresentationController(std::string host,
                                                     std::uint16_t port)
    : impl_(std::make_unique<Impl>(std::move(host), port)) {}
RpcPresentationController::~RpcPresentationController() = default;
RpcPresentationController::RpcPresentationController(
    RpcPresentationController &&) noexcept = default;
RpcPresentationController &RpcPresentationController::operator=(
    RpcPresentationController &&) noexcept = default;

Language::LNode RpcPresentationController::GetActiveTree() const {
  return Wire::DecodeNode(
      impl_->client.call("presentation.getActiveTree").as<std::string>());
}
void RpcPresentationController::SetActiveTree(Language::LNode tree) {
  impl_->client.call("presentation.setActiveTree", Wire::EncodeNode(tree));
}
void RpcPresentationController::AddTree(Language::LNodeID parent,
                                        Language::LNode tree) {
  impl_->client.call("presentation.addTree", parent, Wire::EncodeNode(tree));
}
void RpcPresentationController::UpdateNode(Language::LNodeID node,
                                           Language::LNode replacement) {
  impl_->client.call("presentation.updateNode", node,
                     Wire::EncodeNode(replacement));
}
void RpcPresentationController::Move(Language::LNodeID node,
                                     Language::LNodeID newParent) {
  impl_->client.call("presentation.move", node, newParent);
}
void RpcPresentationController::Remove(Language::LNodeID node) {
  impl_->client.call("presentation.remove", node);
}
void RpcPresentationController::Clear() {
  impl_->client.call("presentation.clear");
}

} // namespace ARUI::Presentation
