#include "VISR/Presentation/PresentationRpcServer.hpp"

#include "Wire.hpp"

#include <rpc/server.h>
#include <string>
#include <utility>
#include <vector>

namespace VISR::Presentation {
class PresentationRpcServer::Impl {
public:
  Impl(IPresentationController &controller, std::uint16_t port)
      : server("127.0.0.1", port) {
    server.bind("presentation.ping", [] { return true; });
    server.bind("presentation.getActiveTree", [&controller] { return Wire::EncodeNode(controller.GetActiveTree()); });
    server.bind("presentation.setActiveTree", [&controller](std::string tree) { controller.SetActiveTree(Wire::DecodeNode(tree)); });
    server.bind("presentation.addTree", [&controller](Language::LNodeID parent, std::string tree) { controller.AddTree(parent, Wire::DecodeNode(tree)); });
    server.bind("presentation.updateNode", [&controller](Language::LNodeID node, std::string replacement) { controller.UpdateNode(node, Wire::DecodeNode(replacement)); });
    server.bind("presentation.move", [&controller](Language::LNodeID node, Language::LNodeID parent) { controller.Move(node, parent); });
    server.bind("presentation.remove", [&controller](Language::LNodeID node) { controller.Remove(node); });
    server.bind("presentation.clear", [&controller] { controller.Clear(); });
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
} // namespace VISR::Presentation
