#include "ARUI/Tools/Simulator/SimulatorApplication.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLOpenXRBinding.hpp"

#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Render/StandardPipeline.hpp"
#include "ARUI/Runtime/Painters/AruiFlatPainter.hpp"
#include "ARUI/Runtime/RuntimePainter.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <glm/geometric.hpp>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace ARUI::Tools::Simulator {
namespace {

float Meters(const std::optional<Language::Length> &length, float fallback) {
  if (!length)
    return fallback;
  switch (length->unit) {
  case Language::LengthUnit::Percent:
    return fallback * static_cast<float>(length->value / 100.0);
  case Language::LengthUnit::Auto:
    return fallback;
  case Language::LengthUnit::Millimeter:
    return static_cast<float>(length->value / 1000.0);
  case Language::LengthUnit::Centimeter:
    return static_cast<float>(length->value / 100.0);
  case Language::LengthUnit::Meter:
    return static_cast<float>(length->value);
  default:
    return fallback;
  }
}

float Meters(const Language::Length &length, float fallback) {
  return Meters(std::optional<Language::Length>{length}, fallback);
}

float InsetMeters(const std::optional<Language::Length> &length,
                  float containingWidth) {
  if (!length || length->IsAuto())
    return 0.0F;
  const float reference =
      length->unit == Language::LengthUnit::Percent ? containingWidth : 0.0F;
  return std::max(0.0F, Meters(length, reference));
}

float Radians(const std::optional<Language::Angle> &angle) {
  if (!angle)
    return 0.0F;
  const float value = static_cast<float>(angle->value);
  return angle->unit == Language::AngleUnit::Degree ? glm::radians(value)
                                                    : value;
}

Render::MeshRenderObject MakeQuad(glm::vec3 center, glm::vec3 right,
                                  glm::vec3 up, float width, float height) {
  const glm::vec3 horizontal = right * width * 0.5F;
  const glm::vec3 vertical = up * height * 0.5F;
  return {.geometry = {.positions = {center - horizontal - vertical,
                                     center + horizontal - vertical,
                                     center + horizontal + vertical,
                                     center - horizontal + vertical},
                       .indices = {0, 1, 2, 2, 3, 0}}};
}

void SubmitTrackedHands(Render::Renderer &renderer, const RenderView &view,
                        SimulatorXRTracker &tracker) {
  static constexpr std::array<std::string_view, 26> joints = {
      "palm",
      "wrist",
      "thumb_metacarpal",
      "thumb_proximal",
      "thumb_distal",
      "thumb_tip",
      "index_metacarpal",
      "index_proximal",
      "index_intermediate",
      "index_distal",
      "index_tip",
      "middle_metacarpal",
      "middle_proximal",
      "middle_intermediate",
      "middle_distal",
      "middle_tip",
      "ring_metacarpal",
      "ring_proximal",
      "ring_intermediate",
      "ring_distal",
      "ring_tip",
      "little_metacarpal",
      "little_proximal",
      "little_intermediate",
      "little_distal",
      "little_tip"};
  static constexpr std::array<std::pair<size_t, size_t>, 25> bones = {
      {{1, 0},   {0, 2},   {2, 3},   {3, 4},   {4, 5},   {0, 6},   {6, 7},
       {7, 8},   {8, 9},   {9, 10},  {0, 11},  {11, 12}, {12, 13}, {13, 14},
       {14, 15}, {0, 16},  {16, 17}, {17, 18}, {18, 19}, {19, 20}, {0, 21},
       {21, 22}, {22, 23}, {23, 24}, {24, 25}}};
  const glm::vec3 cameraRight = glm::vec3{glm::inverse(view.view)[0]};
  const glm::vec3 cameraUp = glm::vec3{glm::inverse(view.view)[1]};
  for (size_t hand = 0; hand < 2; ++hand) {
    if (!tracker.IsHandTracked(hand))
      continue;
    std::array<std::optional<TrackedPose>, joints.size()> poses;
    const std::string prefix = hand == 0 ? "left_hand_" : "right_hand_";
    for (size_t joint = 0; joint < joints.size(); ++joint)
      poses[joint] = tracker.GetPose(prefix + std::string(joints[joint]));
    for (const auto &[start, end] : bones) {
      if (!poses[start] || !poses[end] || !poses[start]->positionValid ||
          !poses[end]->positionValid)
        continue;
      const glm::vec3 from = poses[start]->pose.position;
      const glm::vec3 to = poses[end]->pose.position;
      const glm::vec3 axis = to - from;
      const float length = glm::length(axis);
      if (length < 0.001F)
        continue;
      glm::vec3 side = glm::cross(glm::normalize(axis), cameraRight);
      if (glm::length(side) < 0.1F)
        side = glm::cross(glm::normalize(axis), cameraUp);
      side = glm::normalize(side);
      renderer.Submit(MakeQuad((from + to) * 0.5F, glm::normalize(axis), side,
                               length, 0.008F));
    }
    for (const auto &joint : poses) {
      if (!joint || !joint->positionValid)
        continue;
      renderer.Submit(MakeQuad(joint->pose.position, cameraRight, cameraUp,
                               0.014F, 0.014F));
    }
  }
}

void SubmitTrackedBody(Render::Renderer &renderer, const RenderView &view,
                       SimulatorXRTracker &tracker) {
  if (!tracker.IsBodyTracked())
    return;
  const size_t count = tracker.GetBodyJointCount();
  std::vector<std::optional<TrackedPose>> poses(count);
  for (size_t joint = 0; joint < count; ++joint)
    poses[joint] = tracker.GetBodyJointPose(joint);
  const glm::mat4 cameraWorld = glm::inverse(view.view);
  const glm::vec3 cameraRight = glm::vec3{cameraWorld[0]};
  const glm::vec3 cameraUp = glm::vec3{cameraWorld[1]};
  for (size_t joint = 0; joint < count; ++joint) {
    const auto &pose = poses[joint];
    if (!pose || !pose->positionValid)
      continue;
    // The hand tracker already draws fingers at a finer resolution.
    if (joint < 18 || joint >= 70 || !tracker.IsHandTracked(joint < 44 ? 0 : 1))
      renderer.Submit(
          MakeQuad(pose->pose.position, cameraRight, cameraUp, 0.022F, 0.022F));
    const int32_t parent = tracker.GetBodyJointParent(joint);
    if (parent < 0 || static_cast<size_t>(parent) >= count || !poses[parent] ||
        !poses[parent]->positionValid)
      continue;
    // Avoid drawing the same fingers twice when separate hand tracking is
    // active.
    if (joint >= 18 && joint < 70 && tracker.IsHandTracked(joint < 44 ? 0 : 1))
      continue;
    const glm::vec3 from = poses[parent]->pose.position;
    const glm::vec3 to = pose->pose.position;
    const glm::vec3 axis = to - from;
    const float length = glm::length(axis);
    if (length < 0.001F)
      continue;
    glm::vec3 side = glm::cross(glm::normalize(axis), cameraRight);
    if (glm::length(side) < 0.1F)
      side = glm::cross(glm::normalize(axis), cameraUp);
    side = glm::normalize(side);
    renderer.Submit(MakeQuad((from + to) * 0.5F, glm::normalize(axis), side,
                             length, 0.012F));
  }
}

Runtime::SurfacePaintView MakeSurfaceView(const Runtime::RNode &surface,
                                          SimulatorXRTracker &tracker) {
  const float x = Meters(surface.style.xOffset, 0.0F);
  const float y = Meters(surface.style.yOffset, 0.0F);
  const float z = Meters(surface.style.zOffset, -2.0F);
  const auto anchorAttribute = surface.attributes.find("anchor");
  const auto *anchorName =
      anchorAttribute == surface.attributes.end()
          ? nullptr
          : std::get_if<std::string>(&anchorAttribute->second);
  Pose anchor;
  if (anchorName && *anchorName != "world")
    if (const auto tracked = tracker.GetPose(*anchorName))
      anchor = tracked->pose;
  const glm::vec3 center =
      anchor.position + anchor.orientation * glm::vec3{x, y, z};
  const glm::quat orientation =
      glm::normalize(anchor.orientation *
                     glm::quat(glm::vec3{Radians(surface.style.xRotation),
                                         Radians(surface.style.yRotation),
                                         Radians(surface.style.zRotation)}));
  const glm::mat4 model =
      glm::translate(glm::mat4{1.0F}, center) * glm::mat4_cast(orientation);
  return {.localToWorld = model};
}

} // namespace
SimulatorApplication::SimulatorApplication()
    : tracker_(std::make_shared<SimulatorXRTracker>()),
      views_(std::make_shared<SimulatorViewProvider>(tracker_)),
      presenter_(std::make_shared<SimulatorPresenter>(tracker_, views_)),
      renderDevice_(std::make_shared<Render::OpenGLRenderDevice>(true)) {
  presenter_->SetRenderDevice(renderDevice_);
  renderDevice_->MakeCurrent();
  tracker_->InitializeOpenXR(
      std::make_shared<Render::OpenGLOpenXRBinding>(renderDevice_));
}

int SimulatorApplication::Run() {
  constexpr glm::uvec2 extent{1832, 1920};
  const std::array<glm::uvec2, 3> imageExtents = {
      extent, glm::uvec2{tracker_->GetEyeExtent(0)},
      glm::uvec2{tracker_->GetEyeExtent(1)}};
  std::array<Render::ImageHandle, 3> images;
  std::array<Render::ImageViewHandle, 3> imageViews;
  for (std::size_t i = 0; i < images.size(); ++i) {
    images[i] = renderDevice_->CreateImage(
        {.extent = {imageExtents[i].x, imageExtents[i].y, 1},
         .format = Render::ImageFormat::R8G8B8A8_UNORM,
         .usage = static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::ColorAttachment) |
                  static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::Sampled)});
    imageViews[i] = renderDevice_->CreateImageView(
        {.image = images[i], .format = Render::ImageFormat::R8G8B8A8_UNORM});
  }

  Render::StandardPipeline standardPipeline{*renderDevice_};
  Runtime::PainterRegistry painters;
  Runtime::RegisterAruiFlatPainter(painters);
  using Clock = std::chrono::steady_clock;
  const auto framePeriod = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>{1.0 / 90.0});
  auto nextFrame = Clock::now();
  while (!presenter_->ShouldClose()) {
    tracker_->PollEvents();
    views_->BeginFrame();
    presenter_->BeginFrame();

    if (auto document = presenter_->TakeSubmittedDocument()) {
      auto transaction = runtimeTree_.BeginTransaction();
      const auto oldRoots = runtimeTree_.RootChildren();
      for (const Runtime::NodeID root : oldRoots)
        transaction.Remove(root);
      for (const Language::LNode &surface : *document)
        transaction.InsertTree(runtimeTree_.Root(), surface);
      transaction.Commit();
      presenter_->SetRuntimeRevision(runtimeTree_.Revision());
    }

    const auto frameViews = views_->GetViews();
    for (std::size_t i = 0; i < frameViews.size(); ++i) {
      Render::Renderer renderer(
          *renderDevice_,
          standardPipeline.Configuration(
              {.colorAttachment = imageViews[i],
               .clearColor = true,
               .clearColorValue = i != 0 && tracker_->IsPassthroughEnabled()
                                      ? glm::vec4{0.0F}
                                      : glm::vec4{0.08F, 0.09F, 0.12F, 1.0F},
               .extent = imageExtents[i],
               .offset = {0, 0}},
              frameViews[i].projection * frameViews[i].view));
      Render::RenderGraph graph;
      renderer.BeginFrame();
      Runtime::PaintContext paintContext{renderer};
      for (const Runtime::NodeID root : runtimeTree_.RootChildren()) {
        const auto *surface = runtimeTree_.Get(root);
        if (!surface || surface->surfaceType != Language::SurfaceType::Plane)
          continue;
        Runtime::PaintRuntimeSurface(paintContext, painters, runtimeTree_, root,
                                     MakeSurfaceView(*surface, *tracker_));
      }
      if (tracker_->DrawSkeletons()) {
        SubmitTrackedHands(renderer, frameViews[i], *tracker_);
        SubmitTrackedBody(renderer, frameViews[i], *tracker_);
      }
      renderer.BuildRenderGraph(graph);
      graph.Compile();
      graph.Execute(*renderDevice_);
      renderer.EndFrame();
      presenter_->Present(frameViews[i], imageViews[i]);
    }
    renderDevice_->MakeCurrent();
    tracker_->PresentToHeadset({imageViews[1], imageViews[2]});
    presenter_->EndFrame();
    views_->EndFrame();
    nextFrame += framePeriod;
    const auto now = Clock::now();
    if (nextFrame > now)
      std::this_thread::sleep_until(nextFrame);
    else
      nextFrame = now;
  }

  for (std::size_t i = 0; i < images.size(); ++i) {
    renderDevice_->Destroy(imageViews[i]);
    renderDevice_->Destroy(images[i]);
  }
  return 0;
}

} // namespace ARUI::Tools::Simulator
