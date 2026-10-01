#pragma once

#include "ARUI/Core/Scene/Pose.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace ARUI {

enum class XRDepthImageBackend { OpenGL };

struct XRDepthView {
  Pose pose;
  float angleLeft{};
  float angleRight{};
  float angleUp{};
  float angleDown{};
};

// The native image is owned by the XR runtime and is valid only until the
// current XR frame is submitted. Consumers must not retain it across frames.
struct XRDepthFrame {
  XRDepthImageBackend backend{XRDepthImageBackend::OpenGL};
  std::uint64_t nativeImage{};
  std::uint32_t imageIndex{};
  std::uint32_t width{};
  std::uint32_t height{};
  float nearZ{};
  float farZ{};
  std::int64_t displayTime{};
  std::array<XRDepthView, 2> views{};
};

class IXREnvironment {
public:
  virtual ~IXREnvironment() = default;

  virtual bool SupportsPassthrough() const = 0;
  virtual bool SupportsDepth() const = 0;

  virtual bool IsPassthroughEnabled() const = 0;
  virtual bool SetPassthroughEnabled(bool enabled) = 0;

  virtual std::optional<XRDepthFrame> GetDepthFrame() = 0;
};

} // namespace ARUI
