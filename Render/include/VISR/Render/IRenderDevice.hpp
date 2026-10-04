#pragma once
#include "VISR/Render/IRenderCommandList.hpp"
#include "VISR/Render/RenderCommons.hpp"
#include <memory>
namespace VISR::Render {
class IRenderDevice {
public:
  IRenderDevice() {}
  virtual ~IRenderDevice() = default;
  [[nodiscard]] virtual ImageHandle CreateImage(const ImageDesc &) = 0;
  [[nodiscard]] virtual ImageViewHandle
  CreateImageView(const ImageViewDesc &) = 0;
  [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc &) = 0;
  [[nodiscard]] virtual ShaderModuleHandle
  CreateShaderModule(const ShaderModuleDesc &) = 0;
  [[nodiscard]] virtual GraphicsPipelineHandle
  CreatePipeline(const GraphicsPipelineDesc &) = 0;
  [[nodiscard]] virtual std::unique_ptr<IRenderCommandList>
  CreateCommandList(QueueType type) = 0;

  virtual void Destroy(ImageHandle) = 0;
  virtual void Destroy(ImageViewHandle) = 0;
  virtual void Destroy(BufferHandle) = 0;
  virtual void Destroy(GraphicsPipelineHandle) = 0;
  virtual void Destroy(ShaderModuleHandle) = 0;

  virtual RenderCapabilities GetCapabilities() const = 0;
  virtual void Submit(const IRenderCommandList &commandList) = 0;

protected:
};
} // namespace VISR::Render