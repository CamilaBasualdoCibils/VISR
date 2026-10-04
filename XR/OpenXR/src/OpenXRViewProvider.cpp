#include "VISR/XR/OpenXR/OpenXRViewProvider.hpp"
#include "VISR/XR/OpenXR/OpenXRTrackingProvider.hpp"
#include <stdexcept>

namespace VISR::OpenXR {
OpenXRViewProvider::OpenXRViewProvider(
    std::shared_ptr<OpenXRTrackingProvider> runtime)
    : runtime_(std::move(runtime)) {
  if (!runtime_)
    throw std::invalid_argument("OpenXRViewProvider requires a runtime");
}
void OpenXRViewProvider::BeginFrame() {
  valid_ = runtime_->HasFrameViews();
  if (valid_)
    views_ = runtime_->GetFrameViews();
}
std::span<const RenderView> OpenXRViewProvider::GetViews() const {
  return valid_ ? std::span<const RenderView>{views_}
                : std::span<const RenderView>{};
}
} // namespace VISR::OpenXR
