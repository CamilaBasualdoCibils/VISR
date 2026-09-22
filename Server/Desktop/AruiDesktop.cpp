#include "AruiDesktop.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Render/Backends/Vulkan/VulkanRenderDevice.hpp"
#include "ARUI/Render/RenderCommons.hpp"
#include "ARUI/Render/RenderEnums.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ShaderRegistry.hpp"
#include "ARUI/Tools/Simulator/SimulatorPresenter.hpp"
#include "ARUI/XR/Display/RenderView.hpp"
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

  const Render::GraphicsPipelineHandle pipeline = renderDevice->CreatePipeline(
      Render::GraphicsPipelineDesc{.vertexShader = vertShaderModule,
                                   .fragmentShader = fragShaderModule});

  auto commandList =
      renderDevice->CreateCommandList(Render::QueueType::Graphics);
  commandList->BeginRendering(
      Render::RenderPassDesc{.extent = {800, 600}, .offset = {0, 0}});
  commandList->BindPipeline(pipeline);
  commandList->Draw(Render::PrimitiveTopology::Triangles, 3, 0);
  commandList->EndRendering();
  struct Pass {
    Render::RenderGraphResource texture;
  };
  /* renderGraph->addPass<Pass>(std::string_view name, const Setup &setup,
                             const Execute &execute); */
  /* renderGraph->addPass<Pass>(
      "PassName",
      [](Render::RenderGraph::Builder &builder, Pass &data) {
        data.texture = builder.create<Render::RenderGraphTexture>(
            "sampleTex", Render::RenderGraphTexture::Desc{
                             .extent = glm::ivec3{800, 600, 1},
                             .format = Render::ImageFormat::RGB8});
        data.texture = builder.write(data.texture);
      },
      [&](const Pass &data, FrameGraphPassResources &resources, void *) {
        commandList->BeginRendering(Render::RenderPassDesc{
            .colorAttachment =
                resources.get<Render::RenderGraphTexture>(data.texture).handle},
            .extent = {800, 600},
            .offset = {0, 0},
      }); */
  while (!stopRequested) {
    presenter->BeginFrame();
    renderDevice->Submit(*commandList);
    presenter->Present(RenderView{"Simulator"}, RenderTargetHandle{});
    presenter->EndFrame();
  }

  return 0;
}
