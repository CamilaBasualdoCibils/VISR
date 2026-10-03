#pragma once

#include "ARUI/XR/Environment/IXREnvironment.hpp"
#include "ARUI/Render/RenderCommons.hpp"
#include "ARUI/XR/Display/RenderView.hpp"
#include "ARUI/XR/Tracking/IXRTracker.hpp"

#include <array>
#include <memory>

namespace ARUI::OpenXR {
class OpenXRTrackingProvider;
class IOpenXRGraphicsBinding;
}

namespace ARUI::Tools::Simulator {

class SimulatorXRTracker final : public IXRTracker, public IXREnvironment {
public:
  SimulatorXRTracker();
  ~SimulatorXRTracker() override;
  void InitializeOpenXR(
      std::shared_ptr<OpenXR::IOpenXRGraphicsBinding> graphics);
  [[nodiscard]] glm::ivec2 GetEyeExtent(size_t eye) const noexcept;
  [[nodiscard]] std::optional<RenderView> GetEyeView(size_t eye) const;
  void PresentToHeadset(const std::array<Render::ImageViewHandle, 2> &images);

  std::optional<TrackedPose> GetPose(std::string_view jointName) override;
  size_t GetJoints(std::span<std::string_view> jointNames) override;
  void PollEvents() override;
  [[nodiscard]] bool HasOpenXR() const noexcept;
  [[nodiscard]] bool IsOpenXRRunning() const noexcept;
  bool SupportsPassthrough() const override;
  bool SupportsDepth() const override;
  bool IsPassthroughEnabled() const override;
  bool SetPassthroughEnabled(bool enabled) override;
  std::optional<XRDepthFrame> GetDepthFrame() override;

  [[nodiscard]] bool SupportsHandTracking() const noexcept;
  [[nodiscard]] bool IsHandTracked(size_t hand) const noexcept;
  [[nodiscard]] bool SupportsBodyTracking() const noexcept;
  [[nodiscard]] bool IsBodyTracked() const noexcept;
  [[nodiscard]] float GetBodyConfidence() const noexcept;
  [[nodiscard]] size_t GetBodyJointCount() const noexcept;
  [[nodiscard]] std::string_view GetBodyJointName(size_t joint) const noexcept;
  [[nodiscard]] std::optional<TrackedPose> GetBodyJointPose(size_t joint) const;
  [[nodiscard]] int32_t GetBodyJointParent(size_t joint) const noexcept;
  [[nodiscard]] bool UseOpenXR() const noexcept { return useOpenXR_; }
  void SetUseOpenXR(bool enabled) noexcept { useOpenXR_ = enabled; }
  [[nodiscard]] bool DrawSkeletons() const noexcept { return drawSkeletons_; }
  void SetDrawSkeletons(bool enabled) noexcept { drawSkeletons_ = enabled; }

  TrackedPose &Head() noexcept { return poses_[0]; }
  TrackedPose &LeftHand() noexcept { return poses_[1]; }
  TrackedPose &RightHand() noexcept { return poses_[2]; }

private:
  static constexpr std::array<std::string_view, 3> jointNames_ = {
      "head", "left_hand", "right_hand"};
  std::array<TrackedPose, jointNames_.size()> poses_;
  std::unique_ptr<OpenXR::OpenXRTrackingProvider> openXR_;
  bool useOpenXR_{true};
  bool drawSkeletons_{true};
};

} // namespace ARUI::Tools::Simulator
