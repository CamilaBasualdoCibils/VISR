#pragma once

#include "VISR/Presentation/IPresentationController.hpp"
#include "VISR/Presentation/RpcPresentationController.hpp"

#include <cstddef>
#include <memory>

namespace VISR::Presentation {

class PresentationRpcServer {
public:
  PresentationRpcServer(IPresentationController &controller,
                        std::uint16_t port = DefaultPresentationPort);
  ~PresentationRpcServer();

  PresentationRpcServer(const PresentationRpcServer &) = delete;
  PresentationRpcServer &operator=(const PresentationRpcServer &) = delete;

  void Start(std::size_t workerThreads = 1);
  void Stop();

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace VISR::Presentation
