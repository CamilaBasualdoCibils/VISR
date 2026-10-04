#pragma once

#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/XR/Display/IPresenter.hpp"
#include "ARUI/XR/Display/IViewProvider.hpp"
#include "ARUI/XR/Environment/IXREnvironment.hpp"
#include "ARUI/XR/Tracking/IXRTracker.hpp"

#include <atomic>
#include <memory>

namespace ARUI::Server {
class AruiServer {
public:
  AruiServer(int, char **) {}
  int Run();

  std::shared_ptr<IXRTracker> tracker;
  std::shared_ptr<IXREnvironment> environment;
  std::shared_ptr<IPresenter> presenter;
  std::shared_ptr<IViewProvider> viewProvider;
  std::shared_ptr<Render::IRenderDevice> renderDevice;
  std::shared_ptr<Render::RenderGraph> renderGraph;
  std::atomic_bool stopRequested{false};
};
} // namespace ARUI::Server
