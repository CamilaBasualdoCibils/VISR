#pragma once

#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/Renderer.hpp"

namespace ARUI::Render {

// Shared GPU pipeline used by the viewer, simulator, and server.
class StandardPipeline final {
public:
  explicit StandardPipeline(IRenderDevice &device,
                            ImageFormat colorFormat = ImageFormat::R8G8B8A8_UNORM);
  ~StandardPipeline();
  StandardPipeline(const StandardPipeline &) = delete;
  StandardPipeline &operator=(const StandardPipeline &) = delete;

  [[nodiscard]] RendererConfiguration
  Configuration(RenderPassDesc renderPass) const;

private:
  IRenderDevice *device_{};
  ShaderModuleHandle geometryVertex_;
  ShaderModuleHandle geometryFragment_;
  ShaderModuleHandle textVertex_;
  ShaderModuleHandle textFragment_;
  GraphicsPipelineHandle geometryPipeline_;
  GraphicsPipelineHandle textPipeline_;
};

} // namespace ARUI::Render
