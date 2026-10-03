#include "ARUI/XR/OpenXR/OpenXRViewProvider.hpp"
#include "ARUI/XR/OpenXR/OpenXRTrackingProvider.hpp"
#include <stdexcept>

namespace ARUI::OpenXR {
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
} // namespace ARUI::OpenXR
