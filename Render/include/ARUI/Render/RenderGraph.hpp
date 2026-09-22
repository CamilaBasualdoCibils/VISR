#pragma once

#include "ARUI/Render/RenderCommons.hpp"
#include "fg/Blackboard.hpp"
#include "fg/FrameGraph.hpp"
#include "fg/FrameGraphResource.hpp"
#include <functional>
#include <string_view>
namespace ARUI::Render {
using RenderGraphResource = FrameGraphResource;
using GraphTextureDesc = ImageDesc;
using GraphBufferDesc = BufferDesc;
struct RenderGraphBuffer {
  RenderGraphBuffer() = default;
  using Desc = GraphBufferDesc;
  void create(const Desc &desc, void *) {}
  void destroy(const Desc &desc, void *) {}
  void preRead(const Desc &desc, uint32_t flags, void *context) {}
  void preWrite(const Desc &desc, uint32_t flags, void *context) {}
};
struct RenderGraphTexture {
  RenderGraphTexture() = default;
  using Desc = GraphTextureDesc;
  void create(const Desc &desc, void *) {}
  void destroy(const Desc &desc, void *) {}
  void preRead(const Desc &desc, uint32_t flags, void *context) {}
  void preWrite(const Desc &desc, uint32_t flags, void *context) {}
  ImageHandle handle;
};
struct ImportedRenderGraphTexture {

  using Desc = GraphTextureDesc;
  void create(const Desc &desc, void *) {}
  void destroy(const Desc &desc, void *) {}
  void preRead(const Desc &desc, uint32_t flags, void *context) {}
  void preWrite(const Desc &desc, uint32_t flags, void *context) {}

  ImageHandle handle;
};
class RenderGraph {
public:
  RenderGraph() {}
  ~RenderGraph() {}
  class Builder {
    friend class RenderGraph;

  protected:
    Builder(FrameGraph::Builder &builder) : m_builder(builder) {}

  public:
    template <typename T>
    [[nodiscard]] RenderGraphResource create(const std::string_view name,
                                             const typename T::Desc &desc) {
      return m_builder.create<T>(name, desc);
    }
    RenderGraphResource read(RenderGraphResource resource) {
      return m_builder.read(resource, 0);
    }
    RenderGraphResource write(RenderGraphResource resource) {
      return m_builder.write(resource, 0);
    }
    Builder &setSideEffect() {
      m_builder.setSideEffect();

      return *this;
    }

  private:
    FrameGraph::Builder &m_builder;
  };
  template <typename Data>
  using SetupFunc = std::move_only_function<void(Builder &, Data &data)>;
  template <typename Data>
  using ExecuteFunc = std::move_only_function<void(
      const Data &, FrameGraphPassResources &, void *context)>;
  template <typename Data>
  const Data &addPass(std::string_view name, SetupFunc<Data> &&setup,
                      ExecuteFunc<Data> &&execute) {
    return m_frameGraph.addCallbackPass<Data>(
        name,
        [setup = std::move(setup)](FrameGraph::Builder &builder,
                                   Data &data) mutable {
          Builder m_build(builder);
          setup(m_build, data);
        },
        [execute = std::move(execute)](
            const Data &data, FrameGraphPassResources &resources,
            void *context) mutable { execute(data, resources, context); });
  }
  template <typename T>
  RenderGraphResource Import(const std::string_view name,
                             const typename T::Desc &desc, T &&resource) {
    return m_frameGraph.import(name, desc, std::forward<T>(resource));
  }

private:
  FrameGraph m_frameGraph;
  FrameGraphBlackboard m_blackboard;
};
} // namespace ARUI::Render