#include "ARUI/Tools/Simulator/SimulatorApplication.hpp"

#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ShaderRegistry.hpp"

#include <algorithm>
#include <array>
#include <span>

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

Render::MeshRenderObject MakeQuad(const RenderView &view, glm::vec3 center,
                                  glm::vec3 right, glm::vec3 up, float width,
                                  float height) {
  const glm::vec3 horizontal = right * width * 0.5F;
  const glm::vec3 vertical = up * height * 0.5F;
  const std::array worldPositions = {
      center - horizontal - vertical, center + horizontal - vertical,
      center + horizontal + vertical, center - horizontal + vertical};
  constexpr std::array<uint32_t, 6> indices = {0, 1, 2, 2, 3, 0};
  Render::MeshGeometry geometry;
  geometry.indices.assign(indices.begin(), indices.end());
  geometry.positions.reserve(worldPositions.size());
  const glm::mat4 viewProjection = view.projection * view.view;
  for (const glm::vec3 position : worldPositions) {
    const glm::vec4 clip = viewProjection * glm::vec4(position, 1.0F);
    geometry.positions.emplace_back(glm::vec3{clip} / clip.w);
  }
  return {.geometry = std::move(geometry)};
}

void SubmitRuntimeTree(Render::Renderer &renderer,
                       const Runtime::RuntimeTree &runtime,
                       const RenderView &view, SimulatorXRTracker &tracker) {
  for (const Runtime::NodeID rootID : runtime.RootChildren()) {
    const Runtime::RNode *surface = runtime.Get(rootID);
    if (!surface || !surface->visible ||
        surface->type != Language::LNodeType::Surface ||
        surface->surfaceType != Language::SurfaceType::Plane)
      continue;

    const float x = Meters(surface->style.xOffset, 0.0F);
    const float y = Meters(surface->style.yOffset, 0.0F);
    const float z = Meters(surface->style.zOffset, -2.0F);
    const auto anchorAttribute = surface->attributes.find("anchor");
    const auto *anchorName =
        anchorAttribute == surface->attributes.end()
            ? nullptr
            : std::get_if<std::string>(&anchorAttribute->second);
    Pose anchor;
    if (anchorName && *anchorName != "world") {
      if (const auto trackedPose = tracker.GetPose(*anchorName))
        anchor = trackedPose->pose;
    }
    const glm::vec3 center =
        anchor.position + anchor.orientation * glm::vec3{x, y, z};
    const glm::vec3 right = anchor.Right();
    const glm::vec3 up = anchor.Up();
    const glm::vec3 towardViewer = -anchor.Forward();
    const float width = Meters(surface->style.width, 1.2F);
    const float height = Meters(surface->style.height, 0.7F);
    const float surfacePadding =
        std::max(0.0F, Meters(surface->style.padding, 0.0F));

    constexpr float border = 0.018F;
    renderer.Submit(
        MakeQuad(view, center + up * height * 0.5F, right, up, width, border));
    renderer.Submit(
        MakeQuad(view, center - up * height * 0.5F, right, up, width, border));
    renderer.Submit(MakeQuad(view, center - right * width * 0.5F, right, up,
                             border, height));
    renderer.Submit(MakeQuad(view, center + right * width * 0.5F, right, up,
                             border, height));

    for (const Runtime::NodeID panelID : surface->children) {
      const Runtime::RNode *panel = runtime.Get(panelID);
      if (!panel || !panel->visible)
        continue;
      const float panelMargin =
          std::max(0.0F, Meters(panel->style.margin, 0.0F));
      const float panelWidth = Meters(
          panel->style.width,
          std::max(0.05F, width - 2.0F * (surfacePadding + panelMargin)));
      const float panelHeight = Meters(
          panel->style.height,
          std::max(0.05F, height - 2.0F * (surfacePadding + panelMargin)));
      const glm::vec3 panelCenter = center + towardViewer * 0.01F;
      renderer.Submit(
          MakeQuad(view, panelCenter, right, up, panelWidth, panelHeight));

      const float panelPadding =
          std::max(0.0F, Meters(panel->style.padding, 0.0F));
      for (const Runtime::NodeID childID : panel->children) {
        const Runtime::RNode *child = runtime.Get(childID);
        if (child && child->visible &&
            child->type == Language::LNodeType::Text) {
          const float textWidth = Meters(
              child->style.width,
              std::max(0.02F, panelWidth - 2.0F * panelPadding));
          renderer.Submit(MakeQuad(view, panelCenter + towardViewer * 0.01F,
                                   right, up, textWidth, 0.025F));
        }
      }
    }
  }
}

} // namespace
SimulatorApplication::SimulatorApplication()
    : tracker_(std::make_shared<SimulatorXRTracker>()),
      views_(std::make_shared<SimulatorViewProvider>(tracker_)),
      presenter_(std::make_shared<SimulatorPresenter>(tracker_, views_)),
      renderDevice_(std::make_shared<Render::OpenGLRenderDevice>()) {
  presenter_->SetRenderDevice(renderDevice_);
}

int SimulatorApplication::Run() {
  constexpr glm::uvec2 extent{800, 600};
  std::array<Render::ImageHandle, 3> images;
  std::array<Render::ImageViewHandle, 3> imageViews;
  for (std::size_t i = 0; i < images.size(); ++i) {
    images[i] = renderDevice_->CreateImage(
        {.extent = {extent.x, extent.y, 1},
         .format = Render::ImageFormat::R8G8B8A8_UNORM,
         .usage = static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::ColorAttachment) |
                  static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::Sampled)});
    imageViews[i] = renderDevice_->CreateImageView(
        {.image = images[i], .format = Render::ImageFormat::R8G8B8A8_UNORM});
  }

  // Temporary pipeline for runtime debug geometry. Real runtime rendering will
  // replace this when layout and material resolution are connected.
  const auto &vertexShader = ARUI::Shaders::OpenGL::GetShader("test.vert");
  const auto &fragmentShader = ARUI::Shaders::OpenGL::GetShader("test.frag");
  const auto vertexModule = renderDevice_->CreateShaderModule(
      {.stage = Render::ShaderStageFlags::Vertex,
       .spirv = std::as_bytes(vertexShader.spirv)});
  const auto fragmentModule = renderDevice_->CreateShaderModule(
      {.stage = Render::ShaderStageFlags::Fragment,
       .spirv = std::as_bytes(fragmentShader.spirv)});
  const auto pipeline = renderDevice_->CreatePipeline(
      {.vertexShader = vertexModule,
       .fragmentShader = fragmentModule,
       .vertexLayout =
           {.bindings = {{.binding = 0, .stride = sizeof(glm::vec3)}},
            .attributes = {{.location = 0,
                            .binding = 0,
                            .format = Render::VertexFormat::Float3}}},
       .colorFormats = {Render::ImageFormat::R8G8B8A8_UNORM}});

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
          *renderDevice_, {.pipeline = pipeline,
                           .renderPass = {.colorAttachment = imageViews[i],
                                          .clearColor = true,
                                          .extent = extent,
                                          .offset = {0, 0}}});
      Render::RenderGraph graph;
      renderer.BeginFrame();
      SubmitRuntimeTree(renderer, runtimeTree_, frameViews[i], *tracker_);
      renderer.BuildRenderGraph(graph);
      graph.Compile();
      graph.Execute(*renderDevice_);
      renderer.EndFrame();
      presenter_->Present(frameViews[i], imageViews[i]);
    }
    presenter_->EndFrame();
    views_->EndFrame();
  }

  renderDevice_->Destroy(pipeline);
  renderDevice_->Destroy(fragmentModule);
  renderDevice_->Destroy(vertexModule);
  for (std::size_t i = 0; i < images.size(); ++i) {
    renderDevice_->Destroy(imageViews[i]);
    renderDevice_->Destroy(images[i]);
  }
  return 0;
}

} // namespace ARUI::Tools::Simulator
