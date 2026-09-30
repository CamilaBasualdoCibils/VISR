#include "ARUI/Tools/Simulator/SimulatorXRTracker.hpp"

#include <algorithm>

namespace ARUI::Tools::Simulator {

SimulatorXRTracker::SimulatorXRTracker() {
  for (auto &trackedPose : poses_) {
    trackedPose.positionValid = true;
    trackedPose.orientationValid = true;
    trackedPose.positionTracked = true;
    trackedPose.orientationTracked = true;
  }
  Head().pose.position = {0.0F, 1.7F, 0.0F};
  LeftHand().pose.position = {-0.25F, 1.25F, -0.35F};
  RightHand().pose.position = {0.25F, 1.25F, -0.35F};
}

std::optional<TrackedPose>
SimulatorXRTracker::GetPose(std::string_view jointName) {
  const auto found =
      std::find(jointNames_.begin(), jointNames_.end(), jointName);
  if (found == jointNames_.end())
    return std::nullopt;
  return poses_[static_cast<std::size_t>(found - jointNames_.begin())];
}

size_t SimulatorXRTracker::GetJoints(std::span<std::string_view> jointNames) {
  const auto count = std::min(jointNames.size(), jointNames_.size());
  std::copy_n(jointNames_.begin(), count, jointNames.begin());
  return count;
}

} // namespace ARUI::Tools::Simulator
