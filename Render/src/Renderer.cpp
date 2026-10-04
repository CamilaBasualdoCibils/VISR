#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Render/Font.hpp"

#include "ARUI/Render/IRenderCommandList.hpp"
#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/RenderGraph.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ARUI::Render {
namespace {

struct RenderPassData {
  std::vector<FillPathRenderObject> pathFills;
  std::vector<StrokePathRenderObject> pathStrokes;
  RendererConfiguration configuration;
  std::vector<SurfaceRenderObject> surfaces;
  std::vector<ShapeRenderObject> shapes;
  std::vector<CurveRenderObject> curves;
  std::vector<TextRenderObject> text;
  std::vector<MeshRenderObject> meshes;
  IRenderDevice *expectedDevice{};
};

struct TextVertex {
  glm::vec4 position;
  glm::vec2 uv;
};

glm::vec4 TransformPosition(const glm::mat4 &transform, glm::vec3 point) {
  return transform * glm::vec4{point, 1.0F};
}

struct GeometryBatch {
  std::vector<glm::vec4> vertices;
  std::vector<uint32_t> indices;
};

void AppendMesh(GeometryBatch &batch, const MeshGeometry &mesh,
                const glm::mat4 &localToWorld, const glm::mat4 &worldToClip) {
  if (mesh.positions.empty() || mesh.topology != MeshTopology::Triangles)
    return;

  const auto baseVertex = static_cast<uint32_t>(batch.vertices.size());
  for (const auto &position : mesh.positions)
    batch.vertices.emplace_back(
        TransformPosition(worldToClip * localToWorld, position));

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

void AppendFilledPath(GeometryBatch &batch, const FillPathRenderObject &object,
                      const glm::mat4 &worldToClip) {
  if (!std::holds_alternative<SolidFill>(object.style.fill))
    return;
  std::vector<glm::vec3> polygon;
  for (const auto &command : object.path.commands) {
    if (const auto *move = std::get_if<MoveTo>(&command))
      polygon.emplace_back(move->point, 0.0F);
    else if (const auto *line = std::get_if<LineTo>(&command))
      polygon.emplace_back(line->point, 0.0F);
    else if (!std::holds_alternative<ClosePath>(command))
      return; // Curved-path tessellation policy belongs here, not in painters.
  }
  if (polygon.size() < 3 || !object.path.IsClosed())
    return;
  const auto base = static_cast<uint32_t>(batch.vertices.size());
  for (const auto &point : polygon)
    batch.vertices.emplace_back(
        TransformPosition(worldToClip * object.transform.localToWorld, point));
  for (uint32_t i = 1; i + 1 < polygon.size(); ++i) {
    batch.indices.push_back(base);
    batch.indices.push_back(base + i);
    batch.indices.push_back(base + i + 1);
  }
}

void AppendStrokedPath(GeometryBatch &batch,
                       const StrokePathRenderObject &object,
                       const glm::mat4 &worldToClip) {
  if (!object.style.width.IsAbsolute() || object.style.width.Value() <= 0.0)
    return;
  std::vector<glm::vec2> points;
  for (const auto &command : object.path.commands) {
    if (const auto *move = std::get_if<MoveTo>(&command))
      points.push_back(move->point);
    else if (const auto *line = std::get_if<LineTo>(&command))
      points.push_back(line->point);
    else if (!std::holds_alternative<ClosePath>(command))
      return;
  }
  if (points.size() < 2)
    return;

  const float halfWidth =
      static_cast<float>(
          object.style.width.As(Language::LengthUnit::Meter).Value()) *
      0.5F;
  const bool closed = object.path.IsClosed();
  const std::size_t segmentCount = closed ? points.size() : points.size() - 1;
  const glm::mat4 localToClip = worldToClip * object.transform.localToWorld;
  for (std::size_t i = 0; i < segmentCount; ++i) {
    const glm::vec2 start = points[i];
    const glm::vec2 end = points[(i + 1) % points.size()];
    const glm::vec2 delta = end - start;
    const float length = glm::length(delta);
    if (length <= 1.0e-6F)
      continue;
    const glm::vec2 normal{-delta.y / length * halfWidth,
                           delta.x / length * halfWidth};
    const auto base = static_cast<uint32_t>(batch.vertices.size());
    for (const glm::vec2 point :
         {start + normal, end + normal, end - normal, start - normal})
      batch.vertices.push_back(
          TransformPosition(localToClip, {point.x, point.y, 0.0F}));
    for (const uint32_t index : {0U, 1U, 2U, 2U, 3U, 0U})
      batch.indices.push_back(base + index);
  }
}

void AppendRectangle(GeometryBatch &batch, const RectangleShape &rectangle,
                     const glm::mat4 &localToWorld,
                     const glm::mat4 &worldToClip) {
  const auto baseVertex = static_cast<uint32_t>(batch.vertices.size());
  const glm::vec2 halfSize = rectangle.size * 0.5F;
  constexpr float z = 0.0F;
  for (const glm::vec3 position : {glm::vec3{-halfSize.x, -halfSize.y, z},
                                   glm::vec3{halfSize.x, -halfSize.y, z},
                                   glm::vec3{halfSize.x, halfSize.y, z},
                                   glm::vec3{-halfSize.x, halfSize.y, z}})
    batch.vertices.emplace_back(
        TransformPosition(worldToClip * localToWorld, position));
  for (const uint32_t index : {0U, 1U, 2U, 2U, 3U, 0U})
    batch.indices.push_back(baseVertex + index);
}

} // namespace

Renderer::Renderer(IRenderDevice &device, RendererConfiguration configuration)
    : device_(device), configuration_(configuration) {
  if (configuration_.fontFile.empty())
    configuration_.fontFile = DefaultFontFile();
}

void Renderer::BeginFrame() {
  if (frameActive_)
    throw std::logic_error("renderer frame is already active");
  pathFills_.clear();
  pathStrokes_.clear();
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

void Renderer::FillPath(const FillPathRenderObject &path) {
  RequireSubmissionOpen();
  pathFills_.push_back(path);
}

void Renderer::StrokePath(const StrokePathRenderObject &path) {
  RequireSubmissionOpen();
  pathStrokes_.push_back(path);
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

  RenderPassData submitted{.pathFills = pathFills_,
                           .pathStrokes = pathStrokes_,
                           .configuration = configuration_,
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
        for (const auto &path : data.pathFills)
          AppendFilledPath(batch, path, data.configuration.worldToClip);
        for (const auto &path : data.pathStrokes)
          AppendStrokedPath(batch, path, data.configuration.worldToClip);
        for (const auto &surface : data.surfaces) {
          if (const auto *mesh = std::get_if<MeshSurface>(&surface.geometry))
            AppendMesh(batch, mesh->mesh, surface.transform.localToWorld,
                       data.configuration.worldToClip);
        }
        for (const auto &shape : data.shapes) {
          if (const auto *rectangle =
                  std::get_if<RectangleShape>(&shape.geometry))
            AppendRectangle(batch, *rectangle, shape.transform.localToWorld,
                            data.configuration.worldToClip);
        }
        for (const auto &mesh : data.meshes)
          AppendMesh(batch, mesh.geometry, mesh.transform.localToWorld,
                     data.configuration.worldToClip);

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
          commands->BindVertexBuffer(vertexBuffer, sizeof(glm::vec4));
          commands->BindIndexBuffer(indexBuffer);
          commands->DrawIndexed(static_cast<uint32_t>(batch.indices.size()));
        }

        std::vector<BufferHandle> textBuffers;
        std::vector<ImageHandle> textImages;
        if (data.configuration.textPipeline.value != 0 && !data.text.empty()) {
          commands->BindPipeline(data.configuration.textPipeline);
          for (const auto &text : data.text) {
            if (!text.fontSize.IsAbsolute() || text.fontSize.Value() <= 0.0)
              continue;
            const auto bitmap =
                RasterizeText(text.text, data.configuration.fontFile);
            if (bitmap.Empty())
              continue;
            const float emHeight = static_cast<float>(
                text.fontSize.As(Language::LengthUnit::Meter).Value());
            const float scale = emHeight / static_cast<float>(GlyphPixelHeight);
            const float width = static_cast<float>(bitmap.width) * scale;
            const float height = static_cast<float>(bitmap.height) * scale;
            const glm::mat4 localToClip =
                data.configuration.worldToClip * text.transform.localToWorld;
            const auto transformed = [&](float x, float y) {
              return TransformPosition(localToClip, {x, y, 0.0F});
            };
            const std::array<TextVertex, 6> vertices{{
                {transformed(0.0F, -height), {0.0F, 1.0F}},
                {transformed(width, -height), {1.0F, 1.0F}},
                {transformed(width, 0.0F), {1.0F, 0.0F}},
                {transformed(width, 0.0F), {1.0F, 0.0F}},
                {transformed(0.0F, 0.0F), {0.0F, 0.0F}},
                {transformed(0.0F, -height), {0.0F, 1.0F}},
            }};
            const auto vertexBytes = std::as_bytes(std::span{vertices});
            const auto vertexBuffer = device->CreateBuffer(
                {.size = static_cast<uint32_t>(vertexBytes.size()),
                 .usage =
                     static_cast<BufferUsage>(BufferUsageFlags::VertexBuffer),
                 .initialData = vertexBytes});
            const auto image = device->CreateImage(
                {.extent = {bitmap.width, bitmap.height, 1},
                 .format = ImageFormat::R8_UNORM,
                 .usage = static_cast<ImageUsage>(ImageUsageFlags::Sampled),
                 .initialData = std::span<const std::byte>{bitmap.pixels}});
            textBuffers.push_back(vertexBuffer);
            textImages.push_back(image);
            commands->BindVertexBuffer(vertexBuffer, sizeof(TextVertex));
            commands->BindTexture(0, image);
            commands->Draw(PrimitiveTopology::Triangles, 6, 0);
          }
        }
        commands->EndRendering();
        device->Submit(*commands);
        for (const auto image : textImages)
          device->Destroy(image);
        for (const auto buffer : textBuffers)
          device->Destroy(buffer);
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
  return {.pathFills = pathFills_.size(),
          .pathStrokes = pathStrokes_.size(),
          .surfaces = surfaces_.size(),
          .shapes = shapes_.size(),
          .curves = curves_.size(),
          .text = text_.size(),
          .meshes = meshes_.size()};
}

} // namespace ARUI::Render
