#include "ARUI/Render/Backends/OpenGL/OpenGLCommandList.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"

namespace {
thread_local GLuint activeFramebuffer = 0;
}

void ARUI::Render::OpenGLCommandList::BeginRenderCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
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
    constexpr GLfloat clear[] = {0.08F, 0.09F, 0.12F, 1.0F};
    glClearBufferfv(GL_COLOR, 0, clear);
  }
}

void ARUI::Render::OpenGLCommandList::EndRenderCommand::Execute(
    OpenGLRenderDevice *) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (activeFramebuffer != 0) {
    glDeleteFramebuffers(1, &activeFramebuffer);
    activeFramebuffer = 0;
  }
}

void ARUI::Render::OpenGLCommandList::BindPipelineCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindVertexArray(renderDevice->GetGLPipeline(handle).vaoId);
  glUseProgram(renderDevice->GetGLPipeline(handle).programId);
}

void ARUI::Render::OpenGLCommandList::BindVertexBufferCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindVertexBuffer(0, renderDevice->GetGLBuffer(handle).id, 0, stride);
}

void ARUI::Render::OpenGLCommandList::BindIndexBufferCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderDevice->GetGLBuffer(handle).id);
}

void ARUI::Render::OpenGLCommandList::DrawCommand::Execute(
    OpenGLRenderDevice *) {
  const auto primitive = OpenGL::GetGLPrimitiveTopology(topology);
  if (!primitive)
    throw std::invalid_argument("invalid OpenGL primitive topology");
  glDrawArraysInstancedBaseInstance(*primitive, firstVertex, vertexCount,
                                    instanceCount, firstInstance);
}

void ARUI::Render::OpenGLCommandList::DrawIndexedCommand::Execute(
    OpenGLRenderDevice *) {
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                 GL_UNSIGNED_INT, nullptr);
}
