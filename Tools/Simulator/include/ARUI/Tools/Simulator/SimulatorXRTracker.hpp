#pragma once

#include "ARUI/XR/Tracking/IXRTracker.hpp"
namespace ARUI::Tools::Simulator {
class SimulatorXRTracker : public IXRTracker {

public:
  std::optional<TrackedPose> GetPose(std::string_view jointName) override {}

  size_t GetJoints(std::span<std::string_view> jointNames) override {}

  void PollEvents() override {}
};
} // namespace ARUI::Tools::Simulator
