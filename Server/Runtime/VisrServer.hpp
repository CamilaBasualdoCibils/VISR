#pragma once

#include "VISR/Render/IRenderDevice.hpp"
#include "VISR/Render/RenderGraph.hpp"
#include "VISR/XR/Display/IPresenter.hpp"
#include "VISR/XR/Display/IViewProvider.hpp"
#include "VISR/XR/Environment/IXREnvironment.hpp"
#include "VISR/XR/Tracking/IXRTracker.hpp"

#include <atomic>
#include <memory>

namespace VISR::Server {
class VisrServer {
public:
  VisrServer(int, char **) {}
  int Run();

  std::shared_ptr<IXRTracker> tracker;
  std::shared_ptr<IXREnvironment> environment;
  std::shared_ptr<IPresenter> presenter;
  std::shared_ptr<IViewProvider> viewProvider;
  std::shared_ptr<Render::IRenderDevice> renderDevice;
  std::shared_ptr<Render::RenderGraph> renderGraph;
  std::atomic_bool stopRequested{false};
};
} // namespace VISR::Server
