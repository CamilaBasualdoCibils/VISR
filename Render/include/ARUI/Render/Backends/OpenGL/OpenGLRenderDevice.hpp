#pragma once

#include "ARUI/Render/Backends/OpenGL/OpenGLCommandList.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/RenderCommons.hpp"
#include <EGL/egl.h>
#include <GL/glext.h>
#include <atomic>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <variant>
namespace ARUI::Render {
class OpenGLRenderDevice : public IRenderDevice {

public:
  ImageViewHandle CreateImageView(const ImageViewDesc &desc) override {
    ActivateContext();

    GLenum textureTarget = OpenGL::GetGLImageType(desc.viewType).value();
    GLenum format = OpenGL::GetGLImageFormat(desc.format).value();
    GLuint textureView;
    glGenTextures(1, &textureView);
    glTextureView(textureView, textureTarget, GetGLTexture(desc.image).id,
                  format, desc.baseMipLevel, desc.mipLevelCount,
                  desc.baseArrayLayer, desc.arrayLayerCount);

    ImageViewHandle handle = GenerateTextureViewHandle();

    textureViews[handle] = GLTextureView{textureView, desc.image};

    return handle;
  }

  explicit OpenGLRenderDevice(bool useCurrentContext = false);
  ~OpenGLRenderDevice() override;
  void MakeCurrent() const { ActivateContext(); }
  [[nodiscard]] EGLDisplay EGLDisplayHandle() const { return eglDisplay_; }
  [[nodiscard]] EGLConfig EGLConfigHandle() const { return eglConfig_; }
  [[nodiscard]] EGLContext EGLContextHandle() const { return eglContext_; }
  ImageHandle CreateImage(const ImageDesc &desc) override;

  BufferHandle CreateBuffer(const BufferDesc &desc) override;

  GraphicsPipelineHandle
  CreatePipeline(const GraphicsPipelineDesc &graphicsDesc) override;

  ShaderModuleHandle CreateShaderModule(const ShaderModuleDesc &desc) override;
  void Destroy(GraphicsPipelineHandle handle) override;
  void Destroy(ShaderModuleHandle handle) override;

  void Destroy(ImageHandle handle) override;

  void Destroy(BufferHandle handle) override;
  void Destroy(ImageViewHandle handle) override {
    ActivateContext();
    glDeleteTextures(1, &textureViews.at(handle).id);
    textureViews.erase(handle);
  }

  std::unique_ptr<IRenderCommandList>
  CreateCommandList(QueueType type) override;
  void Submit(const IRenderCommandList &commandList) override;
  RenderCapabilities GetCapabilities() const override { return capabilities; }

  struct GLTexture {
    GLuint id;
  };
  struct GLTextureView {
    GLuint id;
    ImageHandle sourceImage;
  };
  struct GLBuffer {
    GLuint id;
  };
  struct GLShaderModule {
    GLuint id;
  };
  struct GLPipeline {
    GLuint programId;
    GLuint vaoId;
    bool blending{};
  };

  GLTexture GetGLTexture(ImageHandle handle) const {
    return textures.at(handle);
  }
  GLTextureView GetGLTextureView(ImageViewHandle handle) const {
    return textureViews.at(handle);
  }
  GLBuffer GetGLBuffer(BufferHandle handle) const { return buffers.at(handle); }
  GLShaderModule GetGLShaderModule(ShaderModuleHandle handle) const {
    return shaderModules.at(handle);
  }
  GLPipeline GetGLPipeline(GraphicsPipelineHandle handle) const {
    return pipelines.at(handle);
  }

private:
  void ActivateContext() const;

  EGLDisplay eglDisplay_{EGL_NO_DISPLAY};
  EGLConfig eglConfig_{};
  EGLSurface eglSurface_{EGL_NO_SURFACE};
  EGLContext eglContext_{EGL_NO_CONTEXT};
  bool ownsDisplay_{};

  const RenderCapabilities capabilities{};
  static RenderCapabilities GetGLCapabilities();

  std::unordered_map<ImageHandle, GLTexture> textures;
  std::unordered_map<ImageViewHandle, GLTextureView> textureViews;

  std::unordered_map<BufferHandle, GLBuffer> buffers;
  std::unordered_map<ShaderModuleHandle, GLShaderModule> shaderModules;

  std::unordered_map<GraphicsPipelineHandle, GLPipeline> pipelines;

  std::atomic<ImageHandle::Type> nextTextureHandle{1};
  std::atomic<BufferHandle::Type> nextBufferHandle{1};
  std::atomic<ShaderModuleHandle::Type> nextShaderModuleHandle{1};
  std::atomic<GraphicsPipelineHandle::Type> nextPipelineHandle{1};
  std::atomic<ImageViewHandle::Type> nextTextureViewHandle{1};

  ImageHandle GenerateTextureHandle() { return nextTextureHandle++; }
  ImageViewHandle GenerateTextureViewHandle() {
    return nextTextureViewHandle++;
  }
  GraphicsPipelineHandle GeneratePipelineHandle() {
    return nextPipelineHandle++;
  }

  BufferHandle GenerateBufferHandle() { return nextBufferHandle++; }
  ShaderModuleHandle GenerateShaderModuleHandle() {
    return nextShaderModuleHandle++;
  }
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("OpenGLRenderDevice");
};

} // namespace ARUI::Render
