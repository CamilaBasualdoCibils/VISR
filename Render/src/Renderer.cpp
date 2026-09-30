#include "ARUI/Render/Renderer.hpp"

#include "ARUI/Render/IRenderCommandList.hpp"
#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/RenderGraph.hpp"

#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ARUI::Render {
namespace {

struct RenderPassData {
  RendererConfiguration configuration;
  std::vector<SurfaceRenderObject> surfaces;
  std::vector<ShapeRenderObject> shapes;
  std::vector<CurveRenderObject> curves;
  std::vector<TextRenderObject> text;
  std::vector<MeshRenderObject> meshes;
  IRenderDevice *expectedDevice{};
};

struct GeometryBatch {
  std::vector<glm::vec3> vertices;
  std::vector<uint32_t> indices;
};

void AppendMesh(GeometryBatch &batch, const MeshGeometry &mesh,
                const glm::mat4 &transform) {
  if (mesh.positions.empty() || mesh.topology != MeshTopology::Triangles)
    return;

  const auto baseVertex = static_cast<uint32_t>(batch.vertices.size());
  for (const auto &position : mesh.positions)
    batch.vertices.emplace_back(transform * glm::vec4(position, 1.0F));

  if (mesh.indices.empty()) {
    for (uint32_t i = 0; i < mesh.positions.size(); ++i)
      batch.indices.push_back(baseVertex + i);
    return;
  }

  for (const uint32_t index : mesh.indices) {
    if (index >= mesh.positions.size())
      throw std::out_of_range("mesh index exceeds vertex count");
    batch.indices.push_back(baseVertex + index);
  }
}

void AppendRectangle(GeometryBatch &batch, const RectangleShape &rectangle,
                     const glm::mat4 &transform) {
  const auto baseVertex = static_cast<uint32_t>(batch.vertices.size());
  const glm::vec2 halfSize = rectangle.size * 0.5F;
  constexpr float z = 0.0F;
  for (const glm::vec3 position : {glm::vec3{-halfSize.x, -halfSize.y, z},
                                   glm::vec3{halfSize.x, -halfSize.y, z},
                                   glm::vec3{halfSize.x, halfSize.y, z},
                                   glm::vec3{-halfSize.x, halfSize.y, z}})
    batch.vertices.emplace_back(transform * glm::vec4(position, 1.0F));
  for (const uint32_t index : {0U, 1U, 2U, 2U, 3U, 0U})
    batch.indices.push_back(baseVertex + index);
}

} // namespace

Renderer::Renderer(IRenderDevice &device, RendererConfiguration configuration)
    : device_(device), configuration_(configuration) {}

void Renderer::BeginFrame() {
  if (frameActive_)
    throw std::logic_error("renderer frame is already active");
  surfaces_.clear();
  shapes_.clear();
  curves_.clear();
  text_.clear();
  meshes_.clear();
  frameActive_ = true;
  graphBuilt_ = false;
}

void Renderer::RequireSubmissionOpen() const {
  if (!frameActive_ || graphBuilt_)
    throw std::logic_error("render submissions require an open, unbuilt frame");
}

void Renderer::Submit(const SurfaceRenderObject &surface) {
  RequireSubmissionOpen();
  surfaces_.push_back(surface);
}

void Renderer::Submit(const ShapeRenderObject &shape) {
  RequireSubmissionOpen();
  shapes_.push_back(shape);
}

void Renderer::Submit(const CurveRenderObject &curve) {
  RequireSubmissionOpen();
  curves_.push_back(curve);
}

void Renderer::Submit(const TextRenderObject &text) {
  RequireSubmissionOpen();
  text_.push_back(text);
}

void Renderer::Submit(const MeshRenderObject &mesh) {
  RequireSubmissionOpen();
  meshes_.push_back(mesh);
}

void Renderer::BuildRenderGraph(RenderGraph &graph) {
  RequireSubmissionOpen();

  RenderPassData submitted{.configuration = configuration_,
                           .surfaces = surfaces_,
                           .shapes = shapes_,
                           .curves = curves_,
                           .text = text_,
                           .meshes = meshes_,
                           .expectedDevice = &device_};

  graph.addPass<RenderPassData>(
      "ARUI high-level primitives",
      [submitted = std::move(submitted)](RenderGraph::Builder &builder,
                                         RenderPassData &data) mutable {
        data = std::move(submitted);
        builder.setSideEffect();
      },
      [](const RenderPassData &data, FrameGraphPassResources &, void *context) {
        auto *device = static_cast<IRenderDevice *>(context);
        if (device == nullptr || device != data.expectedDevice)
          throw std::logic_error("render graph executed with the wrong device");

        auto commands = device->CreateCommandList(QueueType::Graphics);
        commands->BeginRendering(data.configuration.renderPass);
        if (data.configuration.pipeline.value != 0)
          commands->BindPipeline(data.configuration.pipeline);

        GeometryBatch batch;
        for (const auto &surface : data.surfaces) {
          if (const auto *mesh = std::get_if<MeshSurface>(&surface.geometry))
            AppendMesh(batch, mesh->mesh, surface.transform.localToWorld);
        }
        for (const auto &shape : data.shapes) {
          if (const auto *rectangle =
                  std::get_if<RectangleShape>(&shape.geometry))
            AppendRectangle(batch, *rectangle, shape.transform.localToWorld);
        }
        for (const auto &mesh : data.meshes)
          AppendMesh(batch, mesh.geometry, mesh.transform.localToWorld);

        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;
        if (!batch.indices.empty()) {
          const auto vertexBytes = std::as_bytes(std::span{batch.vertices});
          const auto indexBytes = std::as_bytes(std::span{batch.indices});
          vertexBuffer = device->CreateBuffer(
              {.size = static_cast<uint32_t>(vertexBytes.size()),
               .usage =
                   static_cast<BufferUsage>(BufferUsageFlags::VertexBuffer),
               .initialData = vertexBytes});
          indexBuffer = device->CreateBuffer(
              {.size = static_cast<uint32_t>(indexBytes.size()),
               .usage = static_cast<BufferUsage>(BufferUsageFlags::IndexBuffer),
               .initialData = indexBytes});
          commands->BindVertexBuffer(vertexBuffer, sizeof(glm::vec3));
          commands->BindIndexBuffer(indexBuffer);
          commands->DrawIndexed(static_cast<uint32_t>(batch.indices.size()));
        }

        commands->EndRendering();
        device->Submit(*commands);
        if (!batch.indices.empty()) {
          device->Destroy(indexBuffer);
          device->Destroy(vertexBuffer);
        }
      });
  graphBuilt_ = true;
}

void Renderer::EndFrame() {
  if (!frameActive_)
    throw std::logic_error("renderer frame is not active");
  frameActive_ = false;
}

RendererSubmissionCounts Renderer::SubmissionCounts() const noexcept {
  return {.surfaces = surfaces_.size(),
          .shapes = shapes_.size(),
          .curves = curves_.size(),
          .text = text_.size(),
          .meshes = meshes_.size()};
}

} // namespace ARUI::Render
