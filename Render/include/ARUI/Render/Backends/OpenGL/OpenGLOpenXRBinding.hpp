#pragma once
#include "ARUI/XR/OpenXR/IOpenXRGraphicsBinding.hpp"
#include <memory>

namespace ARUI::Render {
class OpenGLRenderDevice;
class OpenGLOpenXRBinding final : public OpenXR::IOpenXRGraphicsBinding {
public:
  explicit OpenGLOpenXRBinding(std::shared_ptr<OpenGLRenderDevice> device);
  ~OpenGLOpenXRBinding() override;
  std::vector<const char *> RequiredInstanceExtensions() const override;
  XrResult CreateSession(XrInstance instance, XrSystemId system,
                         XrSession *session) override;
  int64_t SelectColorFormat(std::span<const int64_t> formats) const override;
  bool EnumerateSwapchainImages(XrSwapchain swapchain,
                                std::size_t view) override;
  bool EnumerateDepthImages(
      PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA enumerate,
      XrEnvironmentDepthSwapchainMETA swapchain) override;
  std::pair<XRDepthImageBackend, std::uint64_t>
  DepthImage(std::uint32_t index) const override;
  bool CopyRenderTarget(std::size_t view, std::uint32_t imageIndex,
                        ImageViewHandle source, std::uint32_t width,
                        std::uint32_t height) override;
private:
  struct State;
  std::shared_ptr<OpenGLRenderDevice> device_;
  std::unique_ptr<State> state_;
};
} // namespace ARUI::Render
