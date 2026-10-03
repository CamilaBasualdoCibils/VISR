#include "AruiDesktop.hpp"
#include "ARUI/Language/node.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLOpenXRBinding.hpp"
#include "ARUI/Render/Painter.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Render/RuntimePainter.hpp"
#include "ARUI/Render/StandardPipeline.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"
#include "ARUI/XR/OpenXR/OpenXRPresenter.hpp"
#include "ARUI/XR/OpenXR/OpenXRTrackingProvider.hpp"
#include "ARUI/XR/OpenXR/OpenXRViewProvider.hpp"

#include <array>

int ARUI::Server::Manager::AruiDesktop::Run() {
  auto openGL = std::make_shared<Render::OpenGLRenderDevice>();
  auto graphics = std::make_shared<Render::OpenGLOpenXRBinding>(openGL);
  auto openXRPresenter =
      std::make_shared<OpenXR::OpenXRPresenter>(graphics);
  auto openXRRuntime = openXRPresenter->Runtime();
  auto openXRViews = std::make_shared<OpenXR::OpenXRViewProvider>(openXRRuntime);
  presenter = openXRPresenter;
  viewProvider = openXRViews;
  tracker = openXRRuntime;
  environment = openXRRuntime;

  renderDevice = openGL;

  std::array<Render::ImageHandle, 2> images;
  std::array<Render::ImageViewHandle, 2> imageViews;
  std::array<glm::uvec2, 2> extents;
  for (std::size_t eye = 0; eye < 2; ++eye) {
    extents[eye] = glm::uvec2{openXRRuntime->GetRecommendedExtent(eye)};
    images[eye] = renderDevice->CreateImage(
        {.extent = {extents[eye], 1},
         .format = Render::ImageFormat::R8G8B8A8_UNORM,
         .usage = static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::ColorAttachment) |
                  static_cast<Render::ImageUsage>(
                      Render::ImageUsageFlags::Sampled)});
    imageViews[eye] = renderDevice->CreateImageView(
        {.image = images[eye],
         .format = Render::ImageFormat::R8G8B8A8_UNORM});
  }

  Render::StandardPipeline standardPipeline{*renderDevice};
  Render::PainterRegistry painters;
  Render::RegisterDefaultPainter(painters);
  Language::Style surfaceStyle;
  surfaceStyle.width = {1.0, Language::LengthUnit::Meter};
  surfaceStyle.height = {0.75, Language::LengthUnit::Meter};
  surfaceStyle.zOffset = {-2.0, Language::LengthUnit::Meter};
  Runtime::RuntimeTree runtime;
  auto transaction = runtime.BeginTransaction();
  transaction.InsertTree(
      runtime.Root(), Language::LSurface({Language::LText("ARUI Server")}, {},
                                        surfaceStyle));
  transaction.Commit();

  while (!stopRequested && !openXRPresenter->ShouldExit()) {
    presenter->BeginFrame();
    viewProvider->BeginFrame();
    const auto views = viewProvider->GetViews();
    for (std::size_t eye = 0; eye < views.size(); ++eye) {
      Render::Renderer renderer(
          *renderDevice,
          standardPipeline.Configuration(
              {.colorAttachment = imageViews[eye],
               .clearColor = true,
               .clearColorValue = environment->IsPassthroughEnabled()
                                      ? glm::vec4{0.0F}
                                      : glm::vec4{0.08F, 0.09F, 0.12F, 1.0F},
               .extent = extents[eye],
               .offset = {0, 0}}));
      Render::RenderGraph graph;
      renderer.BeginFrame();
      Render::PaintContext paintContext{renderer};
      for (const Runtime::NodeID surface : runtime.RootChildren())
        Render::PaintRuntimeSurface(
            paintContext, painters, runtime, surface,
            {.localToClip = views[eye].projection * views[eye].view,
             .pixelsPerMeter = 600.0F});
      renderer.BuildRenderGraph(graph);
      graph.Compile();
      graph.Execute(*renderDevice);
      renderer.EndFrame();
      presenter->Present(views[eye], imageViews[eye]);
    }
    viewProvider->EndFrame();
    presenter->EndFrame();
  }

  for (std::size_t eye = 0; eye < 2; ++eye) {
    renderDevice->Destroy(imageViews[eye]);
    renderDevice->Destroy(images[eye]);
  }
  return 0;
}
