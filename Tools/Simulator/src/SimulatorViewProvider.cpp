#include "ARUI/Tools/Simulator/SimulatorViewProvider.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace ARUI::Tools::Simulator {
namespace {
constexpr glm::ivec2 viewportSize{800, 600};
constexpr float eyeOffset = 0.032F;

RenderView MakeView(std::string name, const Pose &pose) {
  return {.Name = std::move(name),
          .view = glm::inverse(pose.ToMatrix()),
          .projection = glm::perspective(glm::radians(45.0F),
                                         static_cast<float>(viewportSize.x) /
                                             viewportSize.y,
                                         0.1F, 100.0F),
          .viewportSize = viewportSize};
}
} // namespace

SimulatorViewProvider::SimulatorViewProvider(
    std::shared_ptr<SimulatorXRTracker> tracker)
    : tracker_(std::move(tracker)) {
  BeginFrame();
}

void SimulatorViewProvider::BeginFrame() {
  Pose leftEye = tracker_->Head().pose;
  Pose rightEye = tracker_->Head().pose;
  leftEye.position -= leftEye.Right() * eyeOffset;
  rightEye.position += rightEye.Right() * eyeOffset;
  views_[0] = MakeView("Debug Camera", debugCamera_);
  views_[1] = tracker_->GetEyeView(0).value_or(MakeView("Left Eye", leftEye));
  views_[2] = tracker_->GetEyeView(1).value_or(MakeView("Right Eye", rightEye));
}

} // namespace ARUI::Tools::Simulator
