#include "ARUI/Presentation/RpcPresentationController.hpp"

#include "Wire.hpp"

#include <chrono>
#include <rpc/client.h>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace ARUI::Presentation {
namespace {
std::int64_t Duration(Transition transition) {
  return transition.duration.count();
}
int EasingValue(Transition transition) {
  return static_cast<int>(transition.easing);
}
} // namespace

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

PresentationNodeID
RpcPresentationController::CreateNode(PresentationNodeID parent,
                                      const Language::LNode &node,
                                      Transition transition) {
  return impl_->client
      .call("presentation.createNode", parent, Wire::EncodeNode(node),
            Duration(transition), EasingValue(transition))
      .as<PresentationNodeID>();
}
PresentationNodeID
RpcPresentationController::CreateTree(PresentationNodeID parent,
                                      const Language::LNode &tree,
                                      Transition transition) {
  return impl_->client
      .call("presentation.createTree", parent, Wire::EncodeNode(tree),
            Duration(transition), EasingValue(transition))
      .as<PresentationNodeID>();
}
void RpcPresentationController::Remove(PresentationNodeID node,
                                       Transition transition) {
  impl_->client.call("presentation.remove", node, Duration(transition),
                     EasingValue(transition));
}
void RpcPresentationController::Move(PresentationNodeID node,
                                     PresentationNodeID newParent,
                                     Transition transition) {
  impl_->client.call("presentation.move", node, newParent, Duration(transition),
                     EasingValue(transition));
}
void RpcPresentationController::SetStyle(PresentationNodeID node,
                                         Language::Style style,
                                         Transition transition) {
  impl_->client.call("presentation.setStyle", node, Wire::EncodeStyle(style),
                     Duration(transition), EasingValue(transition));
}
void RpcPresentationController::SetVisibility(PresentationNodeID node,
                                              bool visible,
                                              Transition transition) {
  impl_->client.call("presentation.setVisibility", node, visible,
                     Duration(transition), EasingValue(transition));
}
void RpcPresentationController::ReorderChildren(
    PresentationNodeID parent, std::span<const PresentationNodeID> children,
    Transition transition) {
  impl_->client.call(
      "presentation.reorderChildren", parent,
      std::vector<PresentationNodeID>{children.begin(), children.end()},
      Duration(transition), EasingValue(transition));
}
void RpcPresentationController::Reset(Transition transition) {
  impl_->client.call("presentation.reset", Duration(transition),
                     EasingValue(transition));
}
} // namespace ARUI::Presentation
