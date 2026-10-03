#pragma once

#include "ARUI/Language/Diagnostic.hpp"
#include "ARUI/Language/node.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Tools/Simulator/SimulatorViewProvider.hpp"
#include "ARUI/Tools/Simulator/SimulatorXRTracker.hpp"
#include "ARUI/XR/Display/IPresenter.hpp"

#include <array>
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

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
  std::optional<std::vector<Language::LNode>> TakeSubmittedDocument();
  void SetRuntimeRevision(uint64_t revision) noexcept {
    runtimeRevision_ = revision;
  }

private:
  void DrawEditorShell();
  void DrawTrackingInspector();
  void DrawDocumentEditor();
  void ApplyMarkupEditor();
  void RefreshMarkupFromVisualEditor();
  bool DrawLanguageNodeEditor(Language::LNode &node, bool root);
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
  std::vector<Language::LNode> draftDocuments_;
  std::optional<std::vector<Language::LNode>> submittedDocuments_;
  uint64_t runtimeRevision_{};
  bool autoSubmit_{true};
  bool documentChangedThisFrame_{};
  std::string markupSource_;
  std::vector<Language::Diagnostic> markupDiagnostics_;
  bool markupOutOfSync_{};
  std::shared_ptr<SimulatorXRTracker> tracker_;
  std::shared_ptr<SimulatorViewProvider> views_;
  std::shared_ptr<Render::OpenGLRenderDevice> renderDevice_;
  std::shared_ptr<spdlog::logger> logger_ =
      spdlog::stdout_color_mt("Simulator-Presenter");
};

} // namespace ARUI::Tools::Simulator
