#pragma once

#include "ARUI/XR/Environment/IXREnvironment.hpp"
#include "ARUI/XR/Display/RenderView.hpp"
#include "ARUI/XR/Tracking/IXRTracker.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace ARUI::OpenXR {

// Shares the simulator EGL OpenGL context and submits stereo images to OpenXR.
class OpenXRTrackingProvider final : public IXRTracker, public IXREnvironment {
public:
  static std::unique_ptr<OpenXRTrackingProvider> TryCreate();
  ~OpenXRTrackingProvider() override;

  OpenXRTrackingProvider(const OpenXRTrackingProvider &) = delete;
  OpenXRTrackingProvider &operator=(const OpenXRTrackingProvider &) = delete;

  std::optional<TrackedPose> GetPose(std::string_view jointName) override;
  size_t GetJoints(std::span<std::string_view> jointNames) override;
  void PollEvents() override;

  bool SupportsPassthrough() const override;
  bool SupportsDepth() const override;
  bool IsPassthroughEnabled() const override;
  bool SetPassthroughEnabled(bool enabled) override;
  std::optional<XRDepthFrame> GetDepthFrame() override;

  [[nodiscard]] bool IsRunning() const noexcept;
  [[nodiscard]] bool HasFrameViews() const noexcept;
  [[nodiscard]] const std::array<RenderView, 2> &GetFrameViews() const noexcept;
  [[nodiscard]] bool SupportsHandTracking() const noexcept;
  [[nodiscard]] bool IsHandTracked(size_t hand) const noexcept;
  [[nodiscard]] bool SupportsBodyTracking() const noexcept;
  [[nodiscard]] bool IsBodyTracked() const noexcept;
  [[nodiscard]] float GetBodyConfidence() const noexcept;
  [[nodiscard]] size_t GetBodyJointCount() const noexcept;
  [[nodiscard]] std::string_view GetBodyJointName(size_t joint) const noexcept;
  [[nodiscard]] std::optional<TrackedPose> GetBodyJointPose(size_t joint) const;
  [[nodiscard]] int32_t GetBodyJointParent(size_t joint) const noexcept;
  [[nodiscard]] glm::ivec2 GetRecommendedExtent(size_t eye) const noexcept;
  void BeginFrame();
  void PresentFrame(const std::array<uint32_t, 2> &textures);

private:
  OpenXRTrackingProvider();
  bool Initialize();

  struct State;
  std::unique_ptr<State> state_;
};

} // namespace ARUI::OpenXR
