#include "AruiDesktop.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Render/Backends/Vulkan/VulkanRenderDevice.hpp"
#include "ARUI/Render/Painter.hpp"
#include "ARUI/Render/RenderCommons.hpp"
#include "ARUI/Render/RenderEnums.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Tools/Simulator/SimulatorPresenter.hpp"
#include "ARUI/XR/Display/RenderView.hpp"
#include "ShaderRegistry.hpp"
#include "fg/FrameGraphResource.hpp"
#include "test_frag.hpp"
#include "test_vert.hpp"
#include <imgui.h>
#include <implot.h>
#include <span>
int ARUI::Server::Manager::AruiDesktop::Run() {
  presenter = std::make_shared<Tools::Simulator::SimulatorPresenter>();

  // renderDevice =
  // std::make_shared<ARUI::Render::VulkanRenderDevice>(presenter);
  renderDevice = std::make_shared<Render::OpenGLRenderDevice>();
  renderGraph = std::make_shared<ARUI::Render::RenderGraph>();
  const auto vertShader = ARUI::Shaders::OpenGL::GetShader("test.vert");
  const auto fragShader = ARUI::Shaders::OpenGL::GetShader("test.frag");
  const Render::ShaderModuleHandle vertShaderModule =
      renderDevice->CreateShaderModule(Render::ShaderModuleDesc{
          .stage = Render::ShaderStageFlags::Vertex,
          .spirv{std::as_bytes(
              std::span(vertShader.data(), vertShader.wordCount()))}});
  const Render::ShaderModuleHandle fragShaderModule =
      renderDevice->CreateShaderModule(Render::ShaderModuleDesc{
          .stage = Render::ShaderStageFlags::Fragment,
          .spirv{std::as_bytes(
              std::span(fragShader.data(), fragShader.wordCount()))}});

  const Render::GraphicsPipelineHandle pipeline =
      renderDevice->CreatePipeline(Render::GraphicsPipelineDesc{
          .vertexShader = vertShaderModule,
          .fragmentShader = fragShaderModule,
          .vertexLayout = {
              .bindings = {{.binding = 0, .stride = sizeof(glm::vec3)}},
              .attributes = {{.location = 0,
                              .binding = 0,
                              .format = Render::VertexFormat::Float3}}}});

  Render::Renderer renderer(
      *renderDevice, {.pipeline = pipeline,
                      .renderPass = {.extent = {800, 600}, .offset = {0, 0}}});
  Render::PainterRegistry painterRegistry;
  Render::RegisterDefaultPainter(painterRegistry);
  const Render::IPainter *defaultPainter = painterRegistry.Find("default");
  Render::PaintContext paintContext{renderer};
  const Render::PaintNode rootNode{.type = Language::LNodeType::Panel,
                                   .size = {1.0F, 1.0F}};
  while (!stopRequested) {
    presenter->BeginFrame();
    renderGraph = std::make_shared<Render::RenderGraph>();
    renderer.BeginFrame();
    defaultPainter->Paint(rootNode, {}, paintContext);
    renderer.BuildRenderGraph(*renderGraph);
    renderGraph->Compile();
    renderGraph->Execute(*renderDevice);
    renderer.EndFrame();
    presenter->Present(RenderView{"Simulator"}, Render::ImageViewHandle{});
    presenter->EndFrame();
  }

  return 0;
}
