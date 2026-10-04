#include "ARUI/Render/Font.hpp"
#include "ARUI/Render/IRenderDevice.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Runtime/IPainter.hpp"
#include "ARUI/Runtime/RuntimePainter.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"

#include <algorithm>
#include <gtest/gtest.h>

using namespace ARUI::Render;
using namespace ARUI::Runtime;

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
  renderer.Submit(TextRenderObject{.text = "ARUI"});
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

/* TEST(PainterRegistry, RegistersAndFindsDefaultPainter) {
  PainterRegistry registry;
  RegisterFlatPainter(registry);
  const auto *painter = registry.Find("default");
  ASSERT_NE(painter, nullptr);
  ASSERT_NE(painter->StyleSchema().Find("fill"), nullptr);
  ASSERT_NE(painter->StyleSchema().Find("stroke"), nullptr);
  ASSERT_NE(painter->StyleSchema().Find("stroke-width"), nullptr);
  EXPECT_EQ(registry.Find("arui-flat-painter"), painter);
  EXPECT_EQ(registry.Default(), painter);
  EXPECT_TRUE(painter->StyleSchema().Accepts(
      "fill", ARUI::Language::PainterStyleValue{std::string{"none"}}));
  EXPECT_EQ(registry.Find("missing"), nullptr);
}

TEST(PainterRegistry, RejectsDuplicateRegistration) {
  PainterRegistry registry;
  RegisterFlatPainter(registry);
  EXPECT_THROW(RegisterFlatPainter(registry), std::logic_error);
}

TEST(Font, RasterizesNotoSansUtf8ToCoverageBitmap) {
  const auto bitmap = RasterizeText("Hello, ARUI! — café", DefaultFontFile());
  EXPECT_FALSE(bitmap.Empty());
  EXPECT_GT(bitmap.width, 0u);
  EXPECT_GT(bitmap.height, 0u);
  EXPECT_EQ(bitmap.pixels.size(),
            static_cast<std::size_t>(bitmap.width) * bitmap.height);
  EXPECT_NE(std::find_if(bitmap.pixels.begin(), bitmap.pixels.end(),
                         [](std::byte value) { return value != std::byte{}; }),
            bitmap.pixels.end());
}

TEST(Renderer, DrawsSubmittedTextAsUploadedBitmapQuad) {
  RecordingDevice device;
  auto configuration = Configuration();
  configuration.textPipeline = GraphicsPipelineHandle{43};
  Renderer renderer(device, configuration);
  RenderGraph graph;
  renderer.BeginFrame();
  renderer.Submit(
      TextRenderObject{.text = "Hello, ARUI!", .fontSizePixels = 36.0F});
  renderer.BuildRenderGraph(graph);
  graph.Compile();
  graph.Execute(device);
  renderer.EndFrame();
  EXPECT_EQ(device.createdImages, 1u);
  EXPECT_EQ(device.destroyedImages, 1u);
  EXPECT_EQ(device.lastSubmission.bindTextureCount, 1u);
  EXPECT_EQ(device.lastSubmission.drawnVertices, 6u);
}

TEST(DefaultPainter, PlainTextEmitsTextWithoutBackgroundBox) {
  RecordingDevice device;
  Renderer renderer(device, Configuration());
  PainterRegistry registry;
  RegisterFlatPainter(registry);
  renderer.BeginFrame();
  PaintContext context{renderer};
  ARUI::Language::Attributes attributes{{"text", std::string{"Hello, ARUI!"}}};
  registry.Find("default")->Paint({.type = ARUI::Language::LNodeType::Text,
                                   .attributes = &attributes,
                                   .fontSizePixels = 36.0F,
                                   .fontFamily = "Noto Sans"},
                                  {}, context);
  EXPECT_EQ(renderer.SubmissionCounts().text, 1u);
  EXPECT_EQ(renderer.SubmissionCounts().shapes, 0u);
  renderer.EndFrame();
}

namespace {
const IPainter &PanelPainter() {
  static PainterRegistry registry;
  static const IPainter *painter = [] {
    RegisterFlatPainter(registry);
    return registry.Find("arui-flat-painter");
  }();
  return *painter;
}

PaintNode PanelNode() {
  return {.type = ARUI::Language::LNodeType::Panel, .size = {2.0F, 1.0F}};
}
} // namespace

TEST(DefaultPanelPainter, UnstyledPanelEmitsNothing) {
  SemanticRecorder renderer;
  PaintContext context{renderer};
  PanelPainter().Paint(PanelNode(), {}, context);
  EXPECT_TRUE(renderer.fills.empty());
  EXPECT_TRUE(renderer.strokes.empty());
  EXPECT_EQ(renderer.shapes, 0u);
  EXPECT_EQ(renderer.meshes, 0u);
}

TEST(DefaultPanelPainter, FilledPanelEmitsOneSemanticFill) {
  SemanticRecorder renderer;
  PaintContext context{renderer};
  const ARUI::Language::Color dark{{0.1F, 0.2F, 0.3F, 1.0F}};
  PanelPainter().Paint(PanelNode(), {{"fill", dark}}, context);
  ASSERT_EQ(renderer.fills.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<SolidFill>(renderer.fills[0].style.fill));
  EXPECT_TRUE(renderer.strokes.empty());
  EXPECT_EQ(renderer.meshes, 0u);
}

TEST(DefaultPanelPainter, StrokeIsOneClosedPathInPhysicalUnits) {
  SemanticRecorder renderer;
  PaintContext context{renderer};
  const ARUI::Language::Color white{{1.0F, 1.0F, 1.0F, 1.0F}};
  PanelPainter().Paint(
      PanelNode(),
      {{"stroke", white},
       {"stroke-width",
        ARUI::Language::Length{2.0, ARUI::Language::LengthUnit::Millimeter}}},
      context);
  ASSERT_EQ(renderer.strokes.size(), 1u);
  const auto &stroke = renderer.strokes[0];
  EXPECT_TRUE(stroke.path.IsClosed());
  ASSERT_EQ(stroke.path.commands.size(), 5u);
  EXPECT_TRUE(std::holds_alternative<MoveTo>(stroke.path.commands[0]));
  EXPECT_TRUE(std::holds_alternative<LineTo>(stroke.path.commands[1]));
  EXPECT_TRUE(std::holds_alternative<LineTo>(stroke.path.commands[2]));
  EXPECT_TRUE(std::holds_alternative<LineTo>(stroke.path.commands[3]));
  EXPECT_TRUE(std::holds_alternative<ClosePath>(stroke.path.commands[4]));
  EXPECT_EQ(
      stroke.style.width,
      (ARUI::Language::Length{2.0, ARUI::Language::LengthUnit::Millimeter}));
  EXPECT_EQ(renderer.shapes, 0u);
  EXPECT_EQ(renderer.meshes, 0u);
}

TEST(DefaultPanelPainter, FillAndStrokeEmitIndependentConcepts) {
  SemanticRecorder renderer;
  PaintContext context{renderer};
  const ARUI::Language::Color color{{0.5F, 0.5F, 0.5F, 1.0F}};
  PanelPainter().Paint(
      PanelNode(),
      {{"fill", color},
       {"stroke", color},
       {"stroke-width",
        ARUI::Language::Length{1.0, ARUI::Language::LengthUnit::Centimeter}}},
      context);
  EXPECT_EQ(renderer.fills.size(), 1u);
  EXPECT_EQ(renderer.strokes.size(), 1u);
}

TEST(DefaultPanelPainter, ZeroWidthStrokeEmitsNothing) {
  SemanticRecorder renderer;
  PaintContext context{renderer};
  const ARUI::Language::Color white{{1.0F, 1.0F, 1.0F, 1.0F}};
  PanelPainter().Paint(
      PanelNode(),
      {{"stroke", white},
       {"stroke-width",
        ARUI::Language::Length{0.0, ARUI::Language::LengthUnit::Millimeter}}},
      context);
  EXPECT_TRUE(renderer.strokes.empty());
} */

TEST(SemanticPath, PreservesCubicCurvesWithoutFlattening) {
  Path path;
  path.Move({0.0F, 0.0F})
      .Cubic({0.2F, 1.0F}, {0.8F, 1.0F}, {1.0F, 0.0F})
      .Close();
  SemanticRecorder renderer;
  renderer.FillPath({.path = path,
                     .style = {.fill = SolidFill{ARUI::Language::Color{
                                   {1.0F, 1.0F, 1.0F, 1.0F}}}}});
  ASSERT_EQ(renderer.fills.size(), 1u);
  ASSERT_EQ(renderer.fills[0].path.commands.size(), 3u);
  EXPECT_TRUE(
      std::holds_alternative<CubicTo>(renderer.fills[0].path.commands[1]));
  EXPECT_EQ(renderer.meshes, 0u);
}

TEST(Color, ConvertsAuthoredSRGBHexToLinearFloatColor) {
  const auto black = ARUI::Language::ParseSRGBHexColor("#000000");
  ASSERT_TRUE(black);
  EXPECT_FLOAT_EQ(black->rgba.r, 0.0F);
  EXPECT_FLOAT_EQ(black->rgba.a, 1.0F);

  const auto mid = ARUI::Language::ParseSRGBHexColor("#80808080");
  ASSERT_TRUE(mid);
  EXPECT_NEAR(mid->rgba.r, 0.21586F, 0.0001F);
  EXPECT_NEAR(mid->rgba.g, 0.21586F, 0.0001F);
  EXPECT_NEAR(mid->rgba.b, 0.21586F, 0.0001F);
  EXPECT_NEAR(mid->rgba.a, 128.0F / 255.0F, 0.0001F);
}

/* TEST(RuntimePainter, TraversesRuntimeTreeThroughRegisteredSemanticPainters) {
  ARUI::Language::Style surfaceStyle;
  surfaceStyle.width = {0.4, ARUI::Language::LengthUnit::Meter};
  surfaceStyle.height = {0.2, ARUI::Language::LengthUnit::Meter};
  ARUI::Language::Style panelStyle;
  panelStyle.painterProperties["fill"] =
      ARUI::Language::Color{{0.1F, 0.2F, 0.3F, 1.0F}};

  ARUI::Runtime::RuntimeTree runtime;
  auto transaction = runtime.BeginTransaction();
  transaction.InsertTree(
      runtime.Root(),
      ARUI::Language::LSurface(
          {ARUI::Language::LPanel({ARUI::Language::LText("Shared")}, {},
                                  panelStyle)},
          {}, surfaceStyle));
  transaction.Commit();

  PainterRegistry painters;
  RegisterFlatPainter(painters);
  SemanticRecorder renderer;
  PaintContext context{renderer};
  ASSERT_EQ(runtime.RootChildren().size(), 1u);
  PaintRuntimeSurface(
      context, painters, runtime, runtime.RootChildren()[0],
      {.localToClip = glm::mat4{1.0F}, .pixelsPerMeter = 1000.0F});
  EXPECT_EQ(renderer.fills.size(), 1u);
  EXPECT_EQ(renderer.texts, 1u);
  EXPECT_EQ(renderer.meshes, 0u);
  EXPECT_EQ(renderer.shapes, 0u);
}
 */