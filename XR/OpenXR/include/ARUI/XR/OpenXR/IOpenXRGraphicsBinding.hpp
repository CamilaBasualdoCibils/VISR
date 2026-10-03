#pragma once
#include "ARUI/Render/RenderCommons.hpp"
#include "ARUI/XR/Environment/IXREnvironment.hpp"
#include <openxr/openxr.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ARUI::OpenXR {
class IOpenXRGraphicsBinding {
public:
  virtual ~IOpenXRGraphicsBinding() = default;
  virtual std::vector<const char *> RequiredInstanceExtensions() const = 0;
  virtual XrResult CreateSession(XrInstance instance, XrSystemId system,
                                 XrSession *session) = 0;
  virtual int64_t SelectColorFormat(std::span<const int64_t> formats) const = 0;
  virtual bool EnumerateSwapchainImages(XrSwapchain swapchain,
                                        std::size_t view) = 0;
  virtual bool EnumerateDepthImages(
      PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA enumerate,
      XrEnvironmentDepthSwapchainMETA swapchain) = 0;
  virtual std::pair<XRDepthImageBackend, std::uint64_t>
  DepthImage(std::uint32_t index) const = 0;
  virtual bool CopyRenderTarget(std::size_t view, std::uint32_t imageIndex,
                                Render::ImageViewHandle source,
                                std::uint32_t width,
                                std::uint32_t height) = 0;
};
} // namespace ARUI::OpenXR
