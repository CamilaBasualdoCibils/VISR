#include "VISR/Render/Font.hpp"
#include "VISR/Render/IRenderDevice.hpp"
#include "VISR/Render/RenderGraph.hpp"
#include "VISR/Render/Renderer.hpp"
#include "VISR/Runtime/IPainter.hpp"
#include "VISR/Runtime/RuntimePainter.hpp"
#include "VISR/Runtime/RuntimeTree.hpp"

#include <algorithm>
#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <gtest/gtest.h>

using namespace VISR::Render;
using namespace VISR::Runtime;

namespace {
struct RecordedCommands {
  uint32_t beginCount{}, endCount{}, bindPipelineCount{}, drawCount{},
      drawnVertices{}, bindVertexBufferCount{}, bindIndexBufferCount{},
      bindTextureCount{};
};

class RecordingCommandList final : public IRenderCommandList {
public:
  void BeginRendering(const RenderPassDesc &) override { ++record.beginCount; }
  void EndRendering() override { ++record.endCount; }
  void BindPipeline(GraphicsPipelineHandle) override {
    ++record.bindPipelineCount;
  }
  void BindVertexBuffer(BufferHandle, uint32_t) override {
    ++record.bindVertexBufferCount;
  }
  void BindIndexBuffer(BufferHandle) override { ++record.bindIndexBufferCount; }
  void BindTexture(uint32_t, ImageHandle) override {
    ++record.bindTextureCount;
  }
  void Draw(PrimitiveTopology, uint32_t vertexCount, uint32_t, uint32_t,
            uint32_t) override {
    ++record.drawCount;
    record.drawnVertices += vertexCount;
  }
  void DrawIndexed(uint32_t indexCount) override {
    ++record.drawCount;
    record.drawnVertices += indexCount;
  }
  void Dispatch(uint32_t, uint32_t, uint32_t) override {}
  RecordedCommands record;
};

class RecordingDevice final : public IRenderDevice {
public:
  ImageHandle CreateImage(const ImageDesc &) override {
    ++createdImages;
    return ImageHandle{createdImages};
  }
  ImageViewHandle CreateImageView(const ImageViewDesc &) override { return {}; }
  BufferHandle CreateBuffer(const BufferDesc &description) override {
    ++createdBuffers;
    bufferUploads.emplace_back(description.initialData.begin(),
                               description.initialData.end());
    return BufferHandle{createdBuffers};
  }
  ShaderModuleHandle CreateShaderModule(const ShaderModuleDesc &) override {
    return {};
  }
  GraphicsPipelineHandle CreatePipeline(const GraphicsPipelineDesc &) override {
    return {};
  }
  std::unique_ptr<IRenderCommandList> CreateCommandList(QueueType) override {
    ++createdCommandLists;
    return std::make_unique<RecordingCommandList>();
  }
  void Destroy(ImageHandle) override { ++destroyedImages; }
  void Destroy(ImageViewHandle) override {}
  void Destroy(BufferHandle) override { ++destroyedBuffers; }
  void Destroy(GraphicsPipelineHandle) override {}
  void Destroy(ShaderModuleHandle) override {}
  RenderCapabilities GetCapabilities() const override { return {}; }
  void Submit(const IRenderCommandList &commands) override {
    ++submitCount;
    lastSubmission =
        dynamic_cast<const RecordingCommandList &>(commands).record;
  }
  uint32_t createdCommandLists{}, submitCount{}, createdBuffers{},
      destroyedBuffers{}, createdImages{}, destroyedImages{};
  std::vector<std::vector<std::byte>> bufferUploads;
  RecordedCommands lastSubmission;
};

class SemanticRecorder final : public IRenderer {
public:
  void BeginFrame() override {}
  void FillPath(const FillPathRenderObject &path) override {
    fills.push_back(path);
  }
  void StrokePath(const StrokePathRenderObject &path) override {
    strokes.push_back(path);
  }
  void Submit(const SurfaceRenderObject &) override { ++surfaces; }
  void Submit(const ShapeRenderObject &) override { ++shapes; }
  void Submit(const CurveRenderObject &) override { ++curves; }
  void Submit(const TextRenderObject &) override { ++texts; }
  void Submit(const MeshRenderObject &) override { ++meshes; }
  void BuildRenderGraph(RenderGraph &) override {}
  void EndFrame() override {}

  std::vector<FillPathRenderObject> fills;
  std::vector<StrokePathRenderObject> strokes;
  std::size_t surfaces{}, shapes{}, curves{}, texts{}, meshes{};
};

RendererConfiguration Configuration() {
  return {.pipeline = GraphicsPipelineHandle{42},
          .renderPass = {.extent = {800, 600}, .offset = {0, 0}}};
}
} // namespace

TEST(Renderer, CollectsEveryHighLevelPrimitiveWithoutImmediateGpuWork) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  renderer.BeginFrame();
  renderer.Submit(SurfaceRenderObject{.geometry = PlaneSurface{}});
  renderer.Submit(ShapeRenderObject{.geometry = RectangleShape{}});
  renderer.Submit(CurveRenderObject{});
  renderer.Submit(TextRenderObject{.text = "VISR"});
  renderer.Submit(
      MeshRenderObject{.geometry = {.positions = {{-0.5F, -0.5F, 0.0F},
                                                  {0.5F, -0.5F, 0.0F},
                                                  {0.0F, 0.5F, 0.0F}}}});
  const auto counts = renderer.SubmissionCounts();
  EXPECT_EQ(counts.surfaces, 1);
  EXPECT_EQ(counts.shapes, 1);
  EXPECT_EQ(counts.curves, 1);
  EXPECT_EQ(counts.text, 1);
  EXPECT_EQ(counts.meshes, 1);
  EXPECT_EQ(device.createdCommandLists, 0);
  EXPECT_EQ(device.submitCount, 0);
}

TEST(Renderer, BuildsDeferredRenderGraphWork) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  RenderGraph graph;
  renderer.BeginFrame();
  renderer.Submit(
      MeshRenderObject{.geometry = {.positions = {{-0.5F, -0.5F, 0.0F},
                                                  {0.5F, -0.5F, 0.0F},
                                                  {0.0F, 0.5F, 0.0F}}}});
  renderer.BuildRenderGraph(graph);
  EXPECT_EQ(device.createdCommandLists, 0);
  EXPECT_EQ(device.submitCount, 0);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();
  EXPECT_EQ(device.createdCommandLists, 1);
  EXPECT_EQ(device.submitCount, 1);
  EXPECT_EQ(device.lastSubmission.beginCount, 1);
  EXPECT_EQ(device.lastSubmission.bindPipelineCount, 1);
  EXPECT_EQ(device.lastSubmission.drawCount, 1);
  EXPECT_EQ(device.lastSubmission.drawnVertices, 3);
  EXPECT_EQ(device.lastSubmission.endCount, 1);
}

TEST(Renderer, MeshSurfaceUsesTheSameGeometryPath) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  RenderGraph graph;
  MeshGeometry geometry{.positions = {{0.0F, 0.0F, 0.0F},
                                      {1.0F, 0.0F, 0.0F},
                                      {0.0F, 1.0F, 0.0F}}};
  renderer.BeginFrame();
  renderer.Submit(SurfaceRenderObject{
      .geometry = MeshSurface{.mesh = std::move(geometry)}});
  renderer.BuildRenderGraph(graph);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();
  EXPECT_EQ(device.lastSubmission.drawCount, 1);
  EXPECT_EQ(device.lastSubmission.drawnVertices, 3);
}

TEST(Renderer, BatchesRectanglesAndTriangleMeshesIntoOneIndexedDraw) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  RenderGraph graph;
  renderer.BeginFrame();
  renderer.Submit(
      ShapeRenderObject{.geometry = RectangleShape{.size = {2.0F, 1.0F}}});
  renderer.Submit(
      MeshRenderObject{.geometry = {.positions = {{0.0F, 0.0F, 0.0F},
                                                  {1.0F, 0.0F, 0.0F},
                                                  {0.0F, 1.0F, 0.0F}},
                                    .indices = {0, 1, 2}}});
  renderer.BuildRenderGraph(graph);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();

  EXPECT_EQ(device.createdBuffers, 2);
  EXPECT_EQ(device.destroyedBuffers, 2);
  EXPECT_EQ(device.lastSubmission.bindVertexBufferCount, 1);
  EXPECT_EQ(device.lastSubmission.bindIndexBufferCount, 1);
  EXPECT_EQ(device.lastSubmission.drawCount, 1);
  EXPECT_EQ(device.lastSubmission.drawnVertices, 9);
}

TEST(Renderer, AppliesViewProjectionAfterPainterWorldTransform) {
  RecordingDevice device;
  auto configuration = Configuration();
  configuration.worldToClip = glm::scale(glm::mat4{1.0F}, {2.0F, 3.0F, 1.0F});
  Renderer renderer(device, configuration);
  RenderGraph graph;
  renderer.BeginFrame();
  renderer.Submit(ShapeRenderObject{
      .geometry = RectangleShape{.size = {2.0F, 2.0F}},
      .transform = {.localToWorld =
                        glm::translate(glm::mat4{1.0F}, {3.0F, 4.0F, -2.0F})}});
  renderer.BuildRenderGraph(graph);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();

  ASSERT_GE(device.bufferUploads.size(), 1U);
  ASSERT_GE(device.bufferUploads[0].size(), sizeof(glm::vec4));
  glm::vec4 firstVertex;
  std::memcpy(&firstVertex, device.bufferUploads[0].data(),
              sizeof(firstVertex));
  EXPECT_EQ(firstVertex, (glm::vec4{4.0F, 9.0F, -2.0F, 1.0F}));
}

TEST(Renderer, RasterizesPhysicalTextAndTransformsItsQuad) {
  RecordingDevice device;
  auto configuration = Configuration();
  configuration.textPipeline = GraphicsPipelineHandle{43};
  configuration.worldToClip =
      glm::translate(glm::mat4{1.0F}, {0.25F, -0.5F, 0.0F});
  Renderer renderer(device, configuration);
  RenderGraph graph;
  renderer.BeginFrame();
  renderer.Submit(TextRenderObject{
      .text = "VISR",
      .fontSize = {6.0, VISR::Language::LengthUnit::Millimeter},
      .transform = {.localToWorld =
                        glm::translate(glm::mat4{1.0F}, {1.0F, 2.0F, -3.0F})}});
  renderer.BuildRenderGraph(graph);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();

  EXPECT_EQ(device.createdImages, 1U);
  EXPECT_EQ(device.destroyedImages, 1U);
  EXPECT_EQ(device.lastSubmission.bindTextureCount, 1U);
  EXPECT_EQ(device.lastSubmission.drawnVertices, 6U);
  ASSERT_FALSE(device.bufferUploads.empty());
  glm::vec4 firstVertex;
  std::memcpy(&firstVertex, device.bufferUploads.front().data(),
              sizeof(firstVertex));
  EXPECT_NEAR(firstVertex.x, 1.25F, 1.0e-6F);
  EXPECT_LT(firstVertex.y, 1.5F);
  EXPECT_NEAR(firstVertex.z, -3.0F, 1.0e-6F);
  EXPECT_NEAR(firstVertex.w, 1.0F, 1.0e-6F);
}

TEST(Renderer, TessellatesPhysicalStrokePathsInTheConfiguredView) {
  RecordingDevice device;
  auto configuration = Configuration();
  configuration.worldToClip =
      glm::translate(glm::mat4{1.0F}, {0.5F, 0.25F, 0.0F});
  Renderer renderer(device, configuration);
  RenderGraph graph;
  Path path;
  path.Move({0.0F, 0.0F}).Line({1.0F, 0.0F});
  renderer.BeginFrame();
  renderer.StrokePath(
      {.path = std::move(path),
       .style = {.width = {2.0, VISR::Language::LengthUnit::Centimeter}},
       .transform = {.localToWorld = glm::mat4{1.0F}}});
  renderer.BuildRenderGraph(graph);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();

  EXPECT_EQ(device.lastSubmission.drawnVertices, 6U);
  ASSERT_FALSE(device.bufferUploads.empty());
  glm::vec4 firstVertex;
  std::memcpy(&firstVertex, device.bufferUploads.front().data(),
              sizeof(firstVertex));
  EXPECT_NEAR(firstVertex.x, 0.5F, 1.0e-6F);
  EXPECT_NEAR(firstVertex.y, 0.26F, 1.0e-6F);
}

TEST(Renderer, EnforcesFrameSubmissionLifecycle) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  EXPECT_THROW(renderer.Submit(TextRenderObject{}), std::logic_error);
  renderer.BeginFrame();
  EXPECT_THROW(renderer.BeginFrame(), std::logic_error);
  RenderGraph graph;
  renderer.BuildRenderGraph(graph);
  EXPECT_THROW(renderer.Submit(TextRenderObject{}), std::logic_error);
  renderer.EndFrame();
}

TEST(SemanticPath, PreservesCubicCurvesWithoutFlattening) {
  Path path;
  path.Move({0.0F, 0.0F})
      .Cubic({0.2F, 1.0F}, {0.8F, 1.0F}, {1.0F, 0.0F})
      .Close();
  SemanticRecorder renderer;
  renderer.FillPath({.path = path,
                     .style = {.fill = SolidFill{VISR::Language::Color{
                                   {1.0F, 1.0F, 1.0F, 1.0F}}}}});
  ASSERT_EQ(renderer.fills.size(), 1u);
  ASSERT_EQ(renderer.fills[0].path.commands.size(), 3u);
  EXPECT_TRUE(
      std::holds_alternative<CubicTo>(renderer.fills[0].path.commands[1]));
  EXPECT_EQ(renderer.meshes, 0u);
}

TEST(Color, ConvertsAuthoredSRGBHexToLinearFloatColor) {
  const auto black = VISR::Language::ParseSRGBHexColor("#000000");
  ASSERT_TRUE(black);
  EXPECT_FLOAT_EQ(black->rgba.r, 0.0F);
  EXPECT_FLOAT_EQ(black->rgba.a, 1.0F);

  const auto mid = VISR::Language::ParseSRGBHexColor("#80808080");
  ASSERT_TRUE(mid);
  EXPECT_NEAR(mid->rgba.r, 0.21586F, 0.0001F);
  EXPECT_NEAR(mid->rgba.g, 0.21586F, 0.0001F);
  EXPECT_NEAR(mid->rgba.b, 0.21586F, 0.0001F);
  EXPECT_NEAR(mid->rgba.a, 128.0F / 255.0F, 0.0001F);
}
