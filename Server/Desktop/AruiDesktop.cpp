#include "AruiDesktop.hpp"
#include "ARUI/Language/node.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Render/Painter.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Render/RuntimePainter.hpp"
#include "ARUI/Render/StandardPipeline.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"
#include "ARUI/Tools/Simulator/SimulatorPresenter.hpp"

int ARUI::Server::Manager::AruiDesktop::Run() {
  presenter = std::make_shared<Tools::Simulator::SimulatorPresenter>();
  renderDevice = std::make_shared<Render::OpenGLRenderDevice>();

  Render::StandardPipeline standardPipeline{*renderDevice};
  Render::PainterRegistry painters;
  Render::RegisterDefaultPainter(painters);

  Language::Style surfaceStyle;
  surfaceStyle.width = {1.0, Language::LengthUnit::Meter};
  surfaceStyle.height = {0.75, Language::LengthUnit::Meter};
  Runtime::RuntimeTree runtime;
  auto transaction = runtime.BeginTransaction();
  transaction.InsertTree(
      runtime.Root(), Language::LSurface({Language::LText("ARUI Server")}, {},
                                        surfaceStyle));
  transaction.Commit();

  while (!stopRequested) {
    presenter->BeginFrame();
    renderGraph = std::make_shared<Render::RenderGraph>();
    Render::Renderer renderer(
        *renderDevice,
        standardPipeline.Configuration(
            {.clearColor = true,
             .clearColorValue = {0.08F, 0.09F, 0.12F, 1.0F},
             .extent = {800, 600},
             .offset = {0, 0}}));
    renderer.BeginFrame();
    Render::PaintContext paintContext{renderer};
    for (const Runtime::NodeID surface : runtime.RootChildren())
      Render::PaintRuntimeSurface(
          paintContext, painters, runtime, surface,
          {.localToClip = glm::mat4{1.0F}, .pixelsPerMeter = 600.0F});
    renderer.BuildRenderGraph(*renderGraph);
    renderGraph->Compile();
    renderGraph->Execute(*renderDevice);
    renderer.EndFrame();
    presenter->Present(RenderView{"Simulator"}, Render::ImageViewHandle{});
    presenter->EndFrame();
  }
  return 0;
}
