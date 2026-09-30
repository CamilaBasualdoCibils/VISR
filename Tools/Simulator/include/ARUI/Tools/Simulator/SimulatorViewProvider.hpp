#pragma once

#include "ARUI/Tools/Simulator/SimulatorXRTracker.hpp"
#include "ARUI/XR/Display/IViewProvider.hpp"

#include <array>
#include <memory>

namespace ARUI::Tools::Simulator {

class SimulatorViewProvider final : public IViewProvider {
public:
  explicit SimulatorViewProvider(std::shared_ptr<SimulatorXRTracker> tracker);

  void BeginFrame() override;
  std::span<const RenderView> GetViews() const override { return views_; }
  void EndFrame() override {}

  Pose &DebugCamera() noexcept { return debugCamera_; }

private:
  std::shared_ptr<SimulatorXRTracker> tracker_;
  Pose debugCamera_{
      {2.0F, 1.8F, 2.0F},
      glm::quatLookAt(glm::normalize(glm::vec3{-2.0F, -0.3F, -2.0F}),
                      glm::vec3{0.0F, 1.0F, 0.0F})};
  std::array<RenderView, 3> views_;
};

} // namespace ARUI::Tools::Simulator
