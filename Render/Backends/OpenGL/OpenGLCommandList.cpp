#include "VISR/Render/Backends/OpenGL/OpenGLCommandList.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "VISR/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"

#if defined(TRACY_ENABLE)
#include <tracy/TracyOpenGL.hpp>
#endif

namespace {
thread_local GLuint activeFramebuffer = 0;
}

void VISR::Render::OpenGLCommandList::BeginRenderCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
#if defined(TRACY_ENABLE)
  TracyGpuZone("OpenGL Begin Render Pass");
#endif
  if (desc.colorAttachment.value != 0) {
    glCreateFramebuffers(1, &activeFramebuffer);
    const GLuint texture =
        renderDevice->GetGLTextureView(desc.colorAttachment).id;
    glNamedFramebufferTexture(activeFramebuffer, GL_COLOR_ATTACHMENT0, texture,
                              0);
    constexpr GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(activeFramebuffer, 1, &drawBuffer);
    if (glCheckNamedFramebufferStatus(activeFramebuffer, GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error(
          "OpenGL render target framebuffer is incomplete");
    glBindFramebuffer(GL_FRAMEBUFFER, activeFramebuffer);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  glViewport(desc.offset.x, desc.offset.y, desc.extent.x, desc.extent.y);
  if (desc.clearColor) {
    glClearBufferfv(GL_COLOR, 0, &desc.clearColorValue.x);
  }
}

void VISR::Render::OpenGLCommandList::EndRenderCommand::Execute(
    OpenGLRenderDevice *) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (activeFramebuffer != 0) {
    glDeleteFramebuffers(1, &activeFramebuffer);
    activeFramebuffer = 0;
  }
}

void VISR::Render::OpenGLCommandList::BindPipelineCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindVertexArray(renderDevice->GetGLPipeline(handle).vaoId);
  glUseProgram(renderDevice->GetGLPipeline(handle).programId);
  if (renderDevice->GetGLPipeline(handle).blending) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }
}

void VISR::Render::OpenGLCommandList::BindVertexBufferCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindVertexBuffer(0, renderDevice->GetGLBuffer(handle).id, 0, stride);
}

void VISR::Render::OpenGLCommandList::BindIndexBufferCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderDevice->GetGLBuffer(handle).id);
}

void VISR::Render::OpenGLCommandList::BindTextureCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindTextureUnit(slot, renderDevice->GetGLTexture(handle).id);
}

void VISR::Render::OpenGLCommandList::DrawCommand::Execute(
    OpenGLRenderDevice *) {
#if defined(TRACY_ENABLE)
  TracyGpuZone("OpenGL Draw");
#endif
  const auto primitive = OpenGL::GetGLPrimitiveTopology(topology);
  if (!primitive)
    throw std::invalid_argument("invalid OpenGL primitive topology");
  glDrawArraysInstancedBaseInstance(*primitive, firstVertex, vertexCount,
                                    instanceCount, firstInstance);
}

void VISR::Render::OpenGLCommandList::DrawIndexedCommand::Execute(
    OpenGLRenderDevice *) {
#if defined(TRACY_ENABLE)
  TracyGpuZone("OpenGL Draw Indexed");
#endif
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                 GL_UNSIGNED_INT, nullptr);
}
