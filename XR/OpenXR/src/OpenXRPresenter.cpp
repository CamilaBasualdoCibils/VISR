#include "ARUI/XR/OpenXR/OpenXRPresenter.hpp"
#include "ARUI/XR/OpenXR/IOpenXRGraphicsBinding.hpp"
#include "ARUI/XR/OpenXR/OpenXRTrackingProvider.hpp"
#include <stdexcept>

namespace ARUI::OpenXR {
OpenXRPresenter::OpenXRPresenter(
    std::shared_ptr<IOpenXRGraphicsBinding> graphics)
    : graphics_(std::move(graphics)) {
  if (!graphics_)
    throw std::invalid_argument("OpenXRPresenter requires a graphics binding");
  auto runtime = OpenXRTrackingProvider::TryCreate(graphics_);
  if (!runtime)
    throw std::runtime_error("No compatible OpenXR runtime is available");
  runtime_ = std::shared_ptr<OpenXRTrackingProvider>{std::move(runtime)};
}
std::shared_ptr<OpenXRTrackingProvider> OpenXRPresenter::Runtime() const {
  return runtime_;
}
bool OpenXRPresenter::ShouldExit() const noexcept { return runtime_->ShouldExit(); }
void OpenXRPresenter::BeginFrame() {
  hasSubmission_.fill(false);
  runtime_->PollEvents();
  runtime_->BeginFrame();
}
void OpenXRPresenter::Present(const RenderView &view,
                              Render::ImageViewHandle image) {
  const std::size_t eye = view.Name == "Right Eye" ? 1U : 0U;
  submitted_[eye] = image;
  hasSubmission_[eye] = true;
}
void OpenXRPresenter::EndFrame() {
  runtime_->PresentFrame(submitted_, hasSubmission_);
}
std::vector<std::string>
OpenXRPresenter::GetRequiredVulkanExtensions() const { return {}; }
} // namespace ARUI::OpenXR
