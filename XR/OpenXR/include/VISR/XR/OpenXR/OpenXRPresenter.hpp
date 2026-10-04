#pragma once
#include "VISR/XR/Display/IPresenter.hpp"
#include <array>
#include <memory>

namespace VISR::OpenXR {
class IOpenXRGraphicsBinding;
class OpenXRTrackingProvider;
class OpenXRPresenter final : public IPresenter {
public:
  explicit OpenXRPresenter(std::shared_ptr<IOpenXRGraphicsBinding> graphics);
  ~OpenXRPresenter() override = default;
  void BeginFrame() override;
  void Present(const RenderView &view, Render::ImageViewHandle image) override;
  void EndFrame() override;
  std::vector<std::string> GetRequiredVulkanExtensions() const override;
  [[nodiscard]] std::shared_ptr<OpenXRTrackingProvider> Runtime() const;
  [[nodiscard]] bool ShouldExit() const noexcept;
private:
  std::shared_ptr<IOpenXRGraphicsBinding> graphics_;
  std::shared_ptr<OpenXRTrackingProvider> runtime_;
  std::array<Render::ImageViewHandle, 2> submitted_{};
  std::array<bool, 2> hasSubmission_{};
};
} // namespace VISR::OpenXR
