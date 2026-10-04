#pragma once
#include "VISR/Render/RenderCommons.hpp"
#include "VISR/Render/RenderEnums.hpp"
namespace VISR::Render {
class IRenderCommandList {
public:
  virtual ~IRenderCommandList() = default;
  virtual void BeginRendering(const RenderPassDesc &) = 0;
  virtual void EndRendering() = 0;

  virtual void BindPipeline(GraphicsPipelineHandle) = 0;

  virtual void BindVertexBuffer(BufferHandle, uint32_t stride) = 0;
  virtual void BindIndexBuffer(BufferHandle) = 0;
  virtual void BindTexture(uint32_t slot, ImageHandle) = 0;

  virtual void Draw(PrimitiveTopology topology, uint32_t vertexCount,
                    uint32_t firstVertex, uint32_t instanceCount = 1,
                    uint32_t firstInstance = 0) = 0;
  virtual void DrawIndexed(uint32_t indexCount) = 0;

  virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
};
} // namespace VISR::Render