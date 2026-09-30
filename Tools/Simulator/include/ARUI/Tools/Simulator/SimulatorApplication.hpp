#pragma once

#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"
#include "ARUI/Tools/Simulator/SimulatorPresenter.hpp"

#include <memory>

namespace ARUI::Tools::Simulator {

class SimulatorApplication final {
public:
  SimulatorApplication();
  int Run();

private:
  Runtime::RuntimeTree runtimeTree_;
  std::shared_ptr<SimulatorXRTracker> tracker_;
  std::shared_ptr<SimulatorViewProvider> views_;
  std::shared_ptr<SimulatorPresenter> presenter_;
  std::shared_ptr<Render::OpenGLRenderDevice> renderDevice_;
};

} // namespace ARUI::Tools::Simulator
