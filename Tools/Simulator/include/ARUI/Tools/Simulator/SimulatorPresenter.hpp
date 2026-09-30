#pragma once

#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Tools/Simulator/SimulatorViewProvider.hpp"
#include "ARUI/Tools/Simulator/SimulatorXRTracker.hpp"
#include "ARUI/XR/Display/IPresenter.hpp"

#include <array>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ARUI::Tools::Simulator {

class SimulatorPresenter final : public IPresenter {
public:
  SimulatorPresenter();
  SimulatorPresenter(std::shared_ptr<SimulatorXRTracker> tracker,
                     std::shared_ptr<SimulatorViewProvider> views);
  ~SimulatorPresenter() override;

  void BeginFrame() override;
  void Present(const RenderView &view, Render::ImageViewHandle image) override;
  void
  SetRenderDevice(std::shared_ptr<Render::OpenGLRenderDevice> renderDevice);
  void EndFrame() override;
  std::vector<std::string> GetRequiredVulkanExtensions() const override;

  [[nodiscard]] bool ShouldClose() const;

private:
  void DrawEditorShell();
  void DrawTrackingInspector();
  void DrawScenePanel();
  void DrawViewport();
  void DrawConsole();
  void DrawPoseEditor(const char *label, Pose &pose);
  void DrawTrackedPoseEditor(const char *label, TrackedPose &trackedPose);

  struct PresentedView {
    RenderView view;
    Render::ImageViewHandle image;
  };

  GLFWwindow *window{};
  std::array<PresentedView, 3> presentedViews_{};
  std::size_t presentedViewCount_{};
  int selectedView_{};
  bool dockLayoutInitialized_{};
  std::shared_ptr<SimulatorXRTracker> tracker_;
  std::shared_ptr<SimulatorViewProvider> views_;
  std::shared_ptr<Render::OpenGLRenderDevice> renderDevice_;
  std::shared_ptr<spdlog::logger> logger_ =
      spdlog::stdout_color_mt("Simulator-Presenter");
};

} // namespace ARUI::Tools::Simulator
