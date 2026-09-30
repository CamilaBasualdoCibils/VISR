#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Painter.hpp"

#include <gtest/gtest.h>

using namespace ARUI::Render;

namespace {
struct RecordedCommands {
  uint32_t beginCount{}, endCount{}, bindPipelineCount{}, drawCount{},
      drawnVertices{}, bindVertexBufferCount{}, bindIndexBufferCount{};
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
  void BindIndexBuffer(BufferHandle) override {
    ++record.bindIndexBufferCount;
  }
  void BindTexture(uint32_t, ImageHandle) override {}
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
  ImageHandle CreateImage(const ImageDesc &) override { return {}; }
  ImageViewHandle CreateImageView(const ImageViewDesc &) override { return {}; }
  BufferHandle CreateBuffer(const BufferDesc &) override {
    ++createdBuffers;
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
  void Destroy(ImageHandle) override {}
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
      destroyedBuffers{};
  RecordedCommands lastSubmission;
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
  renderer.Submit(TextRenderObject{.text = "ARUI"});
  renderer.Submit(MeshRenderObject{.geometry = {
      .positions = {{-0.5F, -0.5F, 0.0F}, {0.5F, -0.5F, 0.0F},
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
  renderer.Submit(MeshRenderObject{.geometry = {
      .positions = {{-0.5F, -0.5F, 0.0F}, {0.5F, -0.5F, 0.0F},
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
  renderer.Submit(ShapeRenderObject{
      .geometry = RectangleShape{.size = {2.0F, 1.0F}}});
  renderer.Submit(MeshRenderObject{.geometry = {
      .positions = {{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
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

TEST(PainterRegistry, RegistersAndFindsDefaultPainter) {
  PainterRegistry registry;
  RegisterDefaultPainter(registry);
  const auto *painter = registry.Find("default");
  ASSERT_NE(painter, nullptr);
  ASSERT_NE(painter->StyleSchema().Find("color"), nullptr);
  EXPECT_TRUE(painter->StyleSchema().Find("color")->inherited);
  EXPECT_EQ(registry.Find("missing"), nullptr);
}

TEST(PainterRegistry, RejectsDuplicateRegistration) {
  PainterRegistry registry;
  RegisterDefaultPainter(registry);
  EXPECT_THROW(RegisterDefaultPainter(registry), std::logic_error);
}

TEST(DefaultPainter, EmitsThroughPaintContext) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  PainterRegistry registry;
  RegisterDefaultPainter(registry);
  renderer.BeginFrame();
  PaintContext context{renderer};
  registry.Find("default")->Paint(
      {.type = ARUI::Language::LNodeType::Panel, .size = {2.0F, 1.0F}}, {},
      context);
  EXPECT_EQ(renderer.SubmissionCounts().shapes, 1);
  renderer.EndFrame();
}
