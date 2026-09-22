#pragma once

#include "ARUI/XR/Display/IPresenter.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdexcept>
namespace ARUI::Tools::Simulator {
class SimulatorPresenter : public IPresenter {
public:
  std::vector<std::string>
  GetRequiredVulkanExtensions() const override {
    // TODO: Implement this pure virtual method.
    uint32_t count = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<std::string> result;
    for (uint32_t i = 0; i < count; ++i) {
      result.push_back(std::string(extensions[i]));
    }
    return result;
  }

  void BeginFrame() override;

  void Present(const RenderView &view, RenderTargetHandle target) override;

  void EndFrame() override;

  SimulatorPresenter();
  ~SimulatorPresenter();

private:
  void DrawSimulator();
  GLFWwindow *window;
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("Simulator-Presenter");
};
} // namespace ARUI::Tools::Simulator
