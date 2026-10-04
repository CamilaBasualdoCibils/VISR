#pragma once

#include "ARUI/Render/IRenderer.hpp"
#include "ARUI/Render/RenderCommons.hpp"

#include <cstddef>
#include <filesystem>
#include <glm/ext/matrix_float4x4.hpp>
#include <vector>

namespace ARUI::Render {

class IRenderDevice;

struct RendererConfiguration {
  GraphicsPipelineHandle pipeline;
  GraphicsPipelineHandle textPipeline;
  std::filesystem::path fontFile;
  RenderPassDesc renderPass;
  glm::mat4 worldToClip{1.0F};
};

struct RendererSubmissionCounts {
  std::size_t pathFills{};
  std::size_t pathStrokes{};
  std::size_t surfaces{};
  std::size_t shapes{};
  std::size_t curves{};
  std::size_t text{};
  std::size_t meshes{};
};

class Renderer final : public IRenderer {
public:
  Renderer(IRenderDevice &device, RendererConfiguration configuration);

  void BeginFrame() override;
  void FillPath(const FillPathRenderObject &path) override;
  void StrokePath(const StrokePathRenderObject &path) override;
  void Submit(const SurfaceRenderObject &surface) override;
  void Submit(const ShapeRenderObject &shape) override;
  void Submit(const CurveRenderObject &curve) override;
  void Submit(const TextRenderObject &text) override;
  void Submit(const MeshRenderObject &mesh) override;
  void BuildRenderGraph(RenderGraph &graph) override;
  void EndFrame() override;

  [[nodiscard]] RendererSubmissionCounts SubmissionCounts() const noexcept;

private:
  void RequireSubmissionOpen() const;

  IRenderDevice &device_;
  std::vector<FillPathRenderObject> pathFills_;
  std::vector<StrokePathRenderObject> pathStrokes_;
  RendererConfiguration configuration_;
  std::vector<SurfaceRenderObject> surfaces_;
  std::vector<ShapeRenderObject> shapes_;
  std::vector<CurveRenderObject> curves_;
  std::vector<TextRenderObject> text_;
  std::vector<MeshRenderObject> meshes_;
  bool frameActive_{};
  bool graphBuilt_{};
};

} // namespace ARUI::Render
