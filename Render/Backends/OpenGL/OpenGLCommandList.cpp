#include "ARUI/Render/Backends/OpenGL/OpenGLCommandList.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"

void ARUI::Render::OpenGLCommandList::BeginRenderCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  // Implement the execution logic for beginning a render pass.
  glViewport(desc.offset.x, desc.offset.y, desc.extent.x, desc.extent.y);
}
void ARUI::Render::OpenGLCommandList::EndRenderCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  // Implement the execution logic for ending a render pass.
}
void ARUI::Render::OpenGLCommandList::BindPipelineCommand::Execute(
    OpenGLRenderDevice *renderDevice) {
  glBindVertexArray(renderDevice->GetGLPipeline(handle).vaoId);
  glUseProgram(renderDevice->GetGLPipeline(handle).programId);
  // Implement the execution logic for binding a pipeline.
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
    OpenGLRenderDevice *renderDevice) {
  std::optional<GLenum> primitive =
      ARUI::Render::OpenGL::GetGLPrimitiveTopology(topology);
  if (primitive.has_value()) {

    glDrawArraysInstancedBaseInstance(primitive.value(), firstVertex,vertexCount, instanceCount,
                                      firstInstance);
  } else {
    assert(false && "Invalid primitive topology.");
  }
  // Implement the execution logic for drawing.
}

void ARUI::Render::OpenGLCommandList::DrawIndexedCommand::Execute(
    OpenGLRenderDevice *) {
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                 GL_UNSIGNED_INT, nullptr);
}
