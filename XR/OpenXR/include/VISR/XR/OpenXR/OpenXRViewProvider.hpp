#pragma once
#include "VISR/XR/Display/IViewProvider.hpp"
#include <array>
#include <memory>

namespace VISR::OpenXR {
class OpenXRTrackingProvider;
class OpenXRViewProvider final : public IViewProvider {
public:
  explicit OpenXRViewProvider(std::shared_ptr<OpenXRTrackingProvider> runtime);
  void BeginFrame() override;
  std::span<const RenderView> GetViews() const override;
  void EndFrame() override {}
private:
  std::shared_ptr<OpenXRTrackingProvider> runtime_;
  std::array<RenderView, 2> views_{};
  bool valid_{};
};
} // namespace VISR::OpenXR
