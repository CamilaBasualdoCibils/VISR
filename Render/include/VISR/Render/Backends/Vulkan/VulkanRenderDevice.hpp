#pragma once

#include "VISR/Render/IRenderDevice.hpp"
#include "VISR/XR/Display/IPresenter.hpp"
#include "VISR/Render/RenderCommons.hpp"
#include "vulkan/vulkan.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
namespace VISR::Render {
class VulkanRenderDevice : public IRenderDevice {

public:
  VulkanRenderDevice(std::shared_ptr<IPresenter> presenter);
  ImageHandle CreateImage(const ImageDesc &) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `CreateTexture` is not implemented.");
  }

  BufferHandle CreateBuffer(const BufferDesc &) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `CreateBuffer` is not implemented.");
  }

  GraphicsPipelineHandle CreatePipeline(const GraphicsPipelineDesc &) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `CreatePipeline` is not implemented.");
  }

  void Destroy(ImageHandle) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `DestroyTexture` is not implemented.");
  }

  void Destroy(BufferHandle) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `DestroyBuffer` is not implemented.");
  }

  void Destroy(GraphicsPipelineHandle) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `DestroyPipeline` is not implemented.");
  }

  RenderCapabilities GetCapabilities() const override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `GetCapabilities` is not implemented.");
  }

private:
  vk::Instance instance;
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("VulkanRenderDevice");
};
} // namespace VISR::Render
