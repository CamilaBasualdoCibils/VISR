#pragma once

#include "ARUI/XR/Display/IPresenter.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/XR/Tracking/IXRTracker.hpp"
#include "ARUI/XR/Environment/IXREnvironment.hpp"
#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/XR/Display/IViewProvider.hpp"
#include <atomic>
#include <memory>
namespace ARUI::Server::Manager {
class AruiDesktop {

public:
  AruiDesktop(int argc, char **argv) {}
  int Run();

  std::shared_ptr<ARUI::IXRTracker> tracker;
  std::shared_ptr<ARUI::IXREnvironment> environment;
  std::shared_ptr<ARUI::IPresenter> presenter;
  std::shared_ptr<ARUI::IViewProvider> viewProvider;
  std::shared_ptr<ARUI::Render::IRenderDevice> renderDevice;
  std::shared_ptr<ARUI::Render::RenderGraph> renderGraph;
  std::atomic_bool stopRequested{false};
};
} // namespace ARUI::Server::Manager
