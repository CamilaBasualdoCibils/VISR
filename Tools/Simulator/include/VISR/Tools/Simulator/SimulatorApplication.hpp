#pragma once

#include "VISR/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "VISR/Runtime/RuntimeTree.hpp"
#include "VISR/Tools/Simulator/SimulatorPresenter.hpp"

#include <memory>

namespace VISR::Tools::Simulator {

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

} // namespace VISR::Tools::Simulator
