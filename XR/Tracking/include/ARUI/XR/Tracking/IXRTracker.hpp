#pragma once
#include "ARUI/Core/Scene/Pose.hpp"

#include <optional>
#include <span>
#include <string_view>
namespace ARUI {
struct TrackedPose {
  Pose pose;

  bool positionValid = false;
  bool orientationValid = false;

  bool positionTracked = false;
  bool orientationTracked = false;
};
class IXRTracker {
public:
  virtual ~IXRTracker() = default;

  virtual std::optional<TrackedPose> GetPose(std::string_view jointName) = 0;
  virtual size_t GetJoints(std::span<std::string_view> jointNames) = 0;

  virtual void PollEvents() = 0;
};
} // namespace ARUI
