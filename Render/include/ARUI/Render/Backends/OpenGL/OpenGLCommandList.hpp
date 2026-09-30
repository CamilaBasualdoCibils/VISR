#pragma once
#include "ARUI/Render/Backends/OpenGL/OpenGLCommons.hpp"
#include "ARUI/Render/IRenderCommandList.hpp"
#include "ARUI/Render/RenderEnums.hpp"
#include <GL/gl.h>
#include <queue>
#include <vector>
namespace ARUI::Render {
class OpenGLRenderDevice;
class OpenGLCommandList : public IRenderCommandList {
  friend class OpenGLRenderDevice;

public:
  OpenGLCommandList(OpenGLRenderDevice *device) : renderDevice(device) {}
  void BeginRendering(const RenderPassDesc &desc) override {
    commandQueue.push_back(BeginRenderCommand{desc});
  }

  void EndRendering() override { commandQueue.push_back(EndRenderCommand{}); }

  void BindPipeline(GraphicsPipelineHandle handle) override {
    commandQueue.push_back(BindPipelineCommand{handle});
  }

  void BindVertexBuffer(BufferHandle handle, uint32_t stride) override {
    commandQueue.push_back(BindVertexBufferCommand{handle, stride});
  }

  void BindIndexBuffer(BufferHandle handle) override {
    commandQueue.push_back(BindIndexBufferCommand{handle});
  }

  void BindTexture(uint32_t slot, ImageHandle handle) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `BindTexture` is not implemented.");
  }

  void Draw(PrimitiveTopology topology, uint32_t vertexCount,
            uint32_t firstVertex, uint32_t instanceCount = 1,
            uint32_t firstInstance = 0) override {
    commandQueue.push_back(DrawCommand{topology, vertexCount, firstVertex,
                                       instanceCount, firstInstance});
  }

  void DrawIndexed(uint32_t indexCount) override {
    commandQueue.push_back(DrawIndexedCommand{indexCount});
  }

  void Dispatch(uint32_t x, uint32_t y, uint32_t z) override {
    // TODO: Implement this pure virtual method.
    assert(false && "Method `Dispatch` is not implemented.");
  }

  OpenGLCommandList() = default;
  ~OpenGLCommandList() = default;

private:
  OpenGLRenderDevice *renderDevice;
  struct BeginRenderCommand {
    RenderPassDesc desc;
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  struct EndRenderCommand {
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  struct BindPipelineCommand {
    GraphicsPipelineHandle handle;
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  struct BindVertexBufferCommand {
    BufferHandle handle;
    uint32_t stride;
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  struct BindIndexBufferCommand {
    BufferHandle handle;
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  struct DrawCommand {
    PrimitiveTopology topology;
    uint32_t vertexCount;
    uint32_t firstVertex;
    uint32_t instanceCount;
    uint32_t firstInstance;
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  struct DrawIndexedCommand {
    uint32_t indexCount;
    void Execute(OpenGLRenderDevice *renderDevice);
  };
  using Command = std::variant<BeginRenderCommand, EndRenderCommand,
                               BindPipelineCommand, BindVertexBufferCommand,
                               BindIndexBufferCommand, DrawCommand,
                               DrawIndexedCommand>;
  std::vector<Command> commandQueue;

protected:
  std::vector<Command> GetCommandQueue() const { return commandQueue; }
};
} // namespace ARUI::Render