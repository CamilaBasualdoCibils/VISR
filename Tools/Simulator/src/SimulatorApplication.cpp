#include "ARUI/Tools/Simulator/SimulatorApplication.hpp"

#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"

#include <array>

namespace ARUI::Tools::Simulator {

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

  while (!presenter_->ShouldClose()) {
    tracker_->PollEvents();
    views_->BeginFrame();
    presenter_->BeginFrame();
    const auto frameViews = views_->GetViews();
    for (std::size_t i = 0; i < frameViews.size(); ++i) {
      Render::Renderer renderer(
          *renderDevice_, {.renderPass = {.colorAttachment = imageViews[i],
                                          .clearColor = true,
                                          .extent = extent,
                                          .offset = {0, 0}}});
      Render::RenderGraph graph;
      renderer.BeginFrame();
      renderer.BuildRenderGraph(graph);
      graph.Compile();
      graph.Execute(*renderDevice_);
      renderer.EndFrame();
      presenter_->Present(frameViews[i], imageViews[i]);
    }
    presenter_->EndFrame();
    views_->EndFrame();
  }

  for (std::size_t i = 0; i < images.size(); ++i) {
    renderDevice_->Destroy(imageViews[i]);
    renderDevice_->Destroy(images[i]);
  }
  return 0;
}

} // namespace ARUI::Tools::Simulator
