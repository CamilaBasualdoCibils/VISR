#pragma once

#include "ARUI/XR/Tracking/IXRTracker.hpp"

#include <array>

namespace ARUI::Tools::Simulator {

class SimulatorXRTracker final : public IXRTracker {
public:
  SimulatorXRTracker();

  std::optional<TrackedPose> GetPose(std::string_view jointName) override;
  size_t GetJoints(std::span<std::string_view> jointNames) override;
  void PollEvents() override {}

  TrackedPose &Head() noexcept { return poses_[0]; }
  TrackedPose &LeftHand() noexcept { return poses_[1]; }
  TrackedPose &RightHand() noexcept { return poses_[2]; }

private:
  static constexpr std::array<std::string_view, 3> jointNames_ = {
      "head", "left_hand", "right_hand"};
  std::array<TrackedPose, jointNames_.size()> poses_;
};

} // namespace ARUI::Tools::Simulator
