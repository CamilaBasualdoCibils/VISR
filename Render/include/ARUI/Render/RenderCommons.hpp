#pragma once

#include "ARUI/Render/RenderEnums.hpp"
#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <glm/glm.hpp>
#include <span>
#include <string_view>
#include <variant>
#include "ARUI/Render/RenderHandle.hpp"
namespace ARUI::Render {



struct ImageTag{};
using ImageHandle = Handle<ImageTag,uint64_t>;

struct ImageDesc {
  glm::uvec3 extent{1, 1, 1};

  ImageFormat format = ImageFormat::Undefiend;

  uint32_t mipLevels = 1;
  uint32_t arrayLayers = 1;
  uint32_t sampleCount = 1;

  ImageUsage usage;
  ImageType type = ImageType::Image2D;
};
struct ImageViewTag{};
using ImageViewHandle = Handle<ImageViewTag,uint64_t>;
struct ImageViewDesc {
  ImageHandle image;

  ImageFormat format = ImageFormat::Undefiend;

  ImageType viewType = ImageType::Image2D;

  uint32_t baseMipLevel = 0;
  uint32_t mipLevelCount = 1;

  uint32_t baseArrayLayer = 0;
  uint32_t arrayLayerCount = 1;
};
struct SamplerTag{};
using SamplerHandle = Handle<SamplerTag,uint64_t>;
struct SamplerDesc {};
struct BufferDesc {
  uint32_t size;
  BufferUsage usage;
  std::span<const std::byte> initialData{};
};
struct BufferTag{};
using BufferHandle = Handle<BufferTag,uint64_t>;

struct ShaderModuleTag{};
using ShaderModuleHandle = Handle<ShaderModuleTag,uint64_t>;
struct ShaderModuleDesc {
  ShaderStageFlags stage;

  // Your canonical compiled representation.
  std::span<const std::byte> spirv;

  std::string_view entryPoint = "main";
};
struct GraphicsPipelineTag{};
using GraphicsPipelineHandle = Handle<GraphicsPipelineTag,uint64_t>;
struct ComputePipelineTag{};
using ComputePipelineHandle = Handle<ComputePipelineTag,uint64_t>;
struct VertexAttribute {
  uint32_t location;
  uint32_t binding;
  VertexFormat format;
  uint32_t offset;
};

struct VertexBinding {
  uint32_t binding;
  uint32_t stride;
  bool perInstance = false;
};

struct VertexLayout {
  std::vector<VertexBinding> bindings;
  std::vector<VertexAttribute> attributes;
};
struct RasterState {
  CullMode cullMode = CullMode::Back;
  FrontFace frontFace = FrontFace::CounterClockwise;
  PolygonMode polygonMode = PolygonMode::Fill;

  bool depthClamp = false;
};
struct DepthStencilState {
  bool depthTest = true;
  bool depthWrite = true;

  CompareOp depthCompare = CompareOp::Less;
};
struct BlendAttachmentState {
  bool enabled = false;

  BlendFactor srcColor = BlendFactor::One;
  BlendFactor dstColor = BlendFactor::Zero;
  BlendOp colorOp = BlendOp::Add;

  BlendFactor srcAlpha = BlendFactor::One;
  BlendFactor dstAlpha = BlendFactor::Zero;
  BlendOp alphaOp = BlendOp::Add;
};

struct BlendState {
  std::vector<BlendAttachmentState> attachments;
};
struct BindingDesc {
  uint32_t binding;
  BindingType type;
  ShaderStageFlags stages;
};
struct BindGroupLayoutDesc {
  std::vector<BindingDesc> bindings;
};
struct GraphicsPipelineDesc {
  ShaderModuleHandle vertexShader;
  ShaderModuleHandle fragmentShader;

  VertexLayout vertexLayout;

  PrimitiveTopology topology = PrimitiveTopology::Triangles;

  RasterState raster{};
  DepthStencilState depthStencil{};
  BlendState blend{};

  std::vector<BindGroupLayoutDesc> bindGroups;

  std::vector<ImageFormat> colorFormats;
  ImageFormat depthFormat = ImageFormat::Undefiend;

  uint32_t sampleCount = 1;
};
struct ComputePipelineDesc {
  ShaderModuleHandle computeShader;
};
struct RenderPassDesc {
  ImageViewHandle colorAttachment;
  ImageViewHandle depthAttachment;

  bool clearColor = false;
  bool clearDepth = false;
  glm::uvec2 extent;
  glm::uvec2 offset;
};
struct RenderCapabilities {
  bool multiView = false;
  bool externalMemory = false;
  bool externalSemaphore = false;
  bool fragmentDensityMap = false;
  bool variableRateShading = false;
  bool dmaBuf = false;
};

} // namespace ARUI::Render