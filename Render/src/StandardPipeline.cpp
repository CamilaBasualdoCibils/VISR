#include "ARUI/Render/StandardPipeline.hpp"
#include "ShaderRegistry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <span>

namespace ARUI::Render {
StandardPipeline::StandardPipeline(IRenderDevice &device, ImageFormat colorFormat)
    : device_(&device) {
  const auto &geometryVertex = ARUI::Shaders::OpenGL::GetShader("test.vert");
  const auto &geometryFragment = ARUI::Shaders::OpenGL::GetShader("test.frag");
  const auto &textVertex = ARUI::Shaders::OpenGL::GetShader("text.vert");
  const auto &textFragment = ARUI::Shaders::OpenGL::GetShader("text.frag");
  geometryVertex_ = device.CreateShaderModule(
      {.stage = ShaderStageFlags::Vertex,
       .spirv = std::as_bytes(geometryVertex.spirv)});
  geometryFragment_ = device.CreateShaderModule(
      {.stage = ShaderStageFlags::Fragment,
       .spirv = std::as_bytes(geometryFragment.spirv)});
  textVertex_ = device.CreateShaderModule(
      {.stage = ShaderStageFlags::Vertex,
       .spirv = std::as_bytes(textVertex.spirv)});
  textFragment_ = device.CreateShaderModule(
      {.stage = ShaderStageFlags::Fragment,
       .spirv = std::as_bytes(textFragment.spirv)});
  geometryPipeline_ = device.CreatePipeline(
      {.vertexShader = geometryVertex_,
       .fragmentShader = geometryFragment_,
       .vertexLayout =
           {.bindings = {{.binding = 0, .stride = sizeof(glm::vec3)}},
            .attributes = {{.location = 0, .binding = 0,
                            .format = VertexFormat::Float3}}},
       .colorFormats = {colorFormat}});
  textPipeline_ = device.CreatePipeline(
      {.vertexShader = textVertex_,
       .fragmentShader = textFragment_,
       .vertexLayout =
           {.bindings = {{.binding = 0, .stride = sizeof(float) * 5}},
            .attributes = {{.location = 0, .binding = 0,
                            .format = VertexFormat::Float3},
                           {.location = 1, .binding = 0,
                            .format = VertexFormat::Float2,
                            .offset = sizeof(float) * 3}}},
       .depthStencil = {.depthTest = false, .depthWrite = false},
       .blend = {.attachments = {{.enabled = true,
                                  .srcColor = BlendFactor::SrcAlpha,
                                  .dstColor = BlendFactor::OneMinusSrcAlpha,
                                  .srcAlpha = BlendFactor::One,
                                  .dstAlpha = BlendFactor::OneMinusSrcAlpha}}},
       .colorFormats = {colorFormat}});
}

StandardPipeline::~StandardPipeline() {
  if (!device_) return;
  device_->Destroy(textPipeline_);
  device_->Destroy(geometryPipeline_);
  device_->Destroy(textFragment_);
  device_->Destroy(textVertex_);
  device_->Destroy(geometryFragment_);
  device_->Destroy(geometryVertex_);
}

RendererConfiguration
StandardPipeline::Configuration(RenderPassDesc renderPass) const {
  return {.pipeline = geometryPipeline_,
          .textPipeline = textPipeline_,
          .renderPass = std::move(renderPass)};
}
} // namespace ARUI::Render
