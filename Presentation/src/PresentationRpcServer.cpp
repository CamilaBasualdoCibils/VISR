#include "ARUI/Presentation/PresentationRpcServer.hpp"

#include "Wire.hpp"

#include <rpc/server.h>
#include <string>
#include <utility>
#include <vector>

namespace ARUI::Presentation {
namespace {
Transition DecodeTransition(std::int64_t duration, int easing) {
  return {.duration = std::chrono::milliseconds{duration},
          .easing = easing == static_cast<int>(Easing::EaseInOut)
                        ? Easing::EaseInOut
                        : Easing::Linear};
}
} // namespace

class PresentationRpcServer::Impl {
public:
  Impl(IPresentationController &controller, std::uint16_t port)
      : server("127.0.0.1", port) {
    server.bind("presentation.ping", [] { return true; });
    server.bind("presentation.createNode",
                [&controller](PresentationNodeID parent, std::string node,
                              std::int64_t duration, int easing) {
                  return controller.CreateNode(
                      parent, Wire::DecodeNode(node),
                      DecodeTransition(duration, easing));
                });
    server.bind("presentation.createTree",
                [&controller](PresentationNodeID parent, std::string tree,
                              std::int64_t duration, int easing) {
                  return controller.CreateTree(
                      parent, Wire::DecodeNode(tree),
                      DecodeTransition(duration, easing));
                });
    server.bind("presentation.remove",
                [&controller](PresentationNodeID node, std::int64_t duration,
                              int easing) {
                  controller.Remove(node, DecodeTransition(duration, easing));
                });
    server.bind("presentation.move", [&controller](PresentationNodeID node,
                                                   PresentationNodeID newParent,
                                                   std::int64_t duration,
                                                   int easing) {
      controller.Move(node, newParent, DecodeTransition(duration, easing));
    });
    server.bind("presentation.setStyle",
                [&controller](PresentationNodeID node, std::string style,
                              std::int64_t duration, int easing) {
                  controller.SetStyle(node, Wire::DecodeStyle(style),
                                      DecodeTransition(duration, easing));
                });
    server.bind("presentation.setVisibility",
                [&controller](PresentationNodeID node, bool visible,
                              std::int64_t duration, int easing) {
                  controller.SetVisibility(node, visible,
                                           DecodeTransition(duration, easing));
                });
    server.bind("presentation.reorderChildren",
                [&controller](PresentationNodeID parent,
                              std::vector<PresentationNodeID> children,
                              std::int64_t duration, int easing) {
                  controller.ReorderChildren(
                      parent, children, DecodeTransition(duration, easing));
                });
    server.bind("presentation.reset",
                [&controller](std::int64_t duration, int easing) {
                  controller.Reset(DecodeTransition(duration, easing));
                });
  }
  rpc::server server;
};

PresentationRpcServer::PresentationRpcServer(
    IPresentationController &controller, std::uint16_t port)
    : impl_(std::make_unique<Impl>(controller, port)) {}
PresentationRpcServer::~PresentationRpcServer() { Stop(); }
void PresentationRpcServer::Start(std::size_t workerThreads) {
  impl_->server.async_run(workerThreads);
}
void PresentationRpcServer::Stop() {
  if (impl_)
    impl_->server.stop();
}
} // namespace ARUI::Presentation
