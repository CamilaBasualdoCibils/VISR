#include "ARUI/Tools/Simulator/SimulatorXRTracker.hpp"
#include "ARUI/XR/OpenXR/OpenXRTrackingProvider.hpp"

#include <algorithm>

namespace ARUI::Tools::Simulator {

SimulatorXRTracker::SimulatorXRTracker() {
  for (auto &trackedPose : poses_) {
    trackedPose.positionValid = true;
    trackedPose.orientationValid = true;
    trackedPose.positionTracked = true;
    trackedPose.orientationTracked = true;
  }
  Head().pose.position = {0.0F, 1.7F, 0.0F};
  LeftHand().pose.position = {-0.25F, 1.25F, -0.35F};
  RightHand().pose.position = {0.25F, 1.25F, -0.35F};
}

SimulatorXRTracker::~SimulatorXRTracker() = default;

void SimulatorXRTracker::InitializeOpenXR(
    std::shared_ptr<OpenXR::IOpenXRGraphicsBinding> graphics) {
  openXR_ = OpenXR::OpenXRTrackingProvider::TryCreate(std::move(graphics));
}

glm::ivec2 SimulatorXRTracker::GetEyeExtent(size_t eye) const noexcept {
  return openXR_ ? openXR_->GetRecommendedExtent(eye) : glm::ivec2{1832, 1920};
}

std::optional<RenderView> SimulatorXRTracker::GetEyeView(size_t eye) const {
  if (!openXR_ || !openXR_->HasFrameViews())
    return std::nullopt;
  return openXR_->GetFrameViews()[eye];
}

void SimulatorXRTracker::PresentToHeadset(
    const std::array<Render::ImageViewHandle, 2> &images) {
  if (openXR_)
    openXR_->PresentFrame(images, {true, true});
}

bool SimulatorXRTracker::HasOpenXR() const noexcept {
  return static_cast<bool>(openXR_);
}

bool SimulatorXRTracker::IsOpenXRRunning() const noexcept {
  return openXR_ && openXR_->IsRunning();
}

bool SimulatorXRTracker::SupportsPassthrough() const {
  return openXR_ && openXR_->SupportsPassthrough();
}

bool SimulatorXRTracker::SupportsDepth() const {
  return openXR_ && openXR_->SupportsDepth();
}

bool SimulatorXRTracker::IsPassthroughEnabled() const {
  return openXR_ && openXR_->IsPassthroughEnabled();
}

bool SimulatorXRTracker::SetPassthroughEnabled(bool enabled) {
  return openXR_ && openXR_->SetPassthroughEnabled(enabled);
}

std::optional<XRDepthFrame> SimulatorXRTracker::GetDepthFrame() {
  return openXR_ ? openXR_->GetDepthFrame() : std::nullopt;
}

bool SimulatorXRTracker::SupportsHandTracking() const noexcept {
  return openXR_ && openXR_->SupportsHandTracking();
}

bool SimulatorXRTracker::IsHandTracked(size_t hand) const noexcept {
  return useOpenXR_ && openXR_ && openXR_->IsHandTracked(hand);
}

bool SimulatorXRTracker::SupportsBodyTracking() const noexcept {
  return openXR_ && openXR_->SupportsBodyTracking();
}

bool SimulatorXRTracker::IsBodyTracked() const noexcept {
  return useOpenXR_ && openXR_ && openXR_->IsBodyTracked();
}

float SimulatorXRTracker::GetBodyConfidence() const noexcept {
  return openXR_ ? openXR_->GetBodyConfidence() : 0.0F;
}

size_t SimulatorXRTracker::GetBodyJointCount() const noexcept {
  return openXR_ ? openXR_->GetBodyJointCount() : 0;
}

std::string_view
SimulatorXRTracker::GetBodyJointName(size_t joint) const noexcept {
  return openXR_ ? openXR_->GetBodyJointName(joint) : std::string_view{};
}

std::optional<TrackedPose>
SimulatorXRTracker::GetBodyJointPose(size_t joint) const {
  return IsBodyTracked() ? openXR_->GetBodyJointPose(joint) : std::nullopt;
}

int32_t SimulatorXRTracker::GetBodyJointParent(size_t joint) const noexcept {
  return openXR_ ? openXR_->GetBodyJointParent(joint) : -1;
}

void SimulatorXRTracker::PollEvents() {
  if (!openXR_)
    return;
  openXR_->PollEvents();
  openXR_->BeginFrame();
  // Acquire available environment depth once this XR frame has begun.
  (void)openXR_->GetDepthFrame();
  if (!useOpenXR_ || !openXR_->IsRunning())
    return;
  for (std::size_t i = 0; i < jointNames_.size(); ++i) {
    const auto incoming = openXR_->GetPose(jointNames_[i]);
    if (!incoming) {
      poses_[i].positionValid = false;
      poses_[i].orientationValid = false;
      poses_[i].positionTracked = false;
      poses_[i].orientationTracked = false;
      continue;
    }
    if (incoming->positionValid)
      poses_[i].pose.position = incoming->pose.position;
    if (incoming->orientationValid)
      poses_[i].pose.orientation = incoming->pose.orientation;
    poses_[i].positionValid = incoming->positionValid;
    poses_[i].orientationValid = incoming->orientationValid;
    poses_[i].positionTracked = incoming->positionTracked;
    poses_[i].orientationTracked = incoming->orientationTracked;
  }
}

std::optional<TrackedPose>
SimulatorXRTracker::GetPose(std::string_view jointName) {
  const auto found =
      std::find(jointNames_.begin(), jointNames_.end(), jointName);
  if (found != jointNames_.end())
    return poses_[static_cast<std::size_t>(found - jointNames_.begin())];
  return useOpenXR_ && openXR_ ? openXR_->GetPose(jointName) : std::nullopt;
}

size_t SimulatorXRTracker::GetJoints(std::span<std::string_view> jointNames) {
  const auto count = std::min(jointNames.size(), jointNames_.size());
  std::copy_n(jointNames_.begin(), count, jointNames.begin());
  return useOpenXR_ && openXR_ ? openXR_->GetJoints(jointNames) : count;
}

} // namespace ARUI::Tools::Simulator
