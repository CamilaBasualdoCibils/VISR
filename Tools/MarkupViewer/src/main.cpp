#include "ARUI/Language/Parser.hpp"
#include "ARUI/Render/Backends/OpenGL/OpenGLRenderDevice.hpp"
#include "ARUI/Render/RenderGraph.hpp"
#include "ARUI/Render/Renderer.hpp"
#include "ARUI/Render/StandardPipeline.hpp"
#include "ARUI/Runtime/IPainter.hpp"
#include "ARUI/Runtime/RuntimePainter.hpp"
#include "ARUI/Runtime/RuntimeTree.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ARUI::Language::LNodeType;
using ARUI::Runtime::NodeID;
using ARUI::Runtime::RNode;
using ARUI::Runtime::RuntimeTree;

constexpr std::string_view DefaultMarkup = R"(<arui>
  <surface width="20cm" height="10cm">
    <text>Hello, ARUI!</text>
  </surface>
</arui>)";
constexpr glm::uvec2 RenderExtent{1280, 720};

class MarkupViewer {
public:
  explicit MarkupViewer(std::filesystem::path initialPath = {})
      : source_(DefaultMarkup) {
    glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
    if (glfwInit() != GLFW_TRUE)
      throw std::runtime_error("GLFW failed to initialize");

    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ =
        glfwCreateWindow(1440, 900, "ARUI Markup Viewer", nullptr, nullptr);
    if (!window_)
      throw std::runtime_error("GLFW failed to create the markup viewer");
    glfwSetWindowUserPointer(window_, this);
    glm::vec2 content_scale;
    glfwGetWindowContentScale(window_, &content_scale.x, &content_scale.y);
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    glfwSetDropCallback(window_, DropCallback);
    ImGui_ImplOpenGL3_Init("#version 450");
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.FontScaleMain = content_scale.y;
    style.WindowRounding = 0.0F;
    style.ChildRounding = 0.0F;
    style.FrameRounding = 3.0F;

    CreateRenderResources();
    if (initialPath.empty())
      ParseAndSubmit();
    else
      OpenPath(initialPath);
  }

  ~MarkupViewer() {
    DestroyRenderResources();
    glfwMakeContextCurrent(window_);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window_)
      glfwDestroyWindow(window_);
    glfwTerminate();
  }

  int Run() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
      PollLiveReload();
      glfwMakeContextCurrent(window_);
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      Draw();
      RenderPreview();
      glfwMakeContextCurrent(window_);
      ImGui::Render();

      int width = 0;
      int height = 0;
      glfwGetFramebufferSize(window_, &width, &height);
      glViewport(0, 0, width, height);
      glClearColor(0, 0, 0, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window_);
    }
    return 0;
  }

private:
  static void DropCallback(GLFWwindow *window, int count, const char **paths) {
    if (count <= 0 || !paths || !paths[0])
      return;
    auto *viewer =
        static_cast<MarkupViewer *>(glfwGetWindowUserPointer(window));
    if (viewer)
      viewer->OpenPath(paths[0]);
  }

  void OpenPath(const std::filesystem::path &path) {
    std::error_code error;
    if (std::filesystem::is_directory(path, error)) {
      folderInput_ = path.string();
      ScanFolder();
      return;
    }
    LoadFile(path);
  }

  void ScanFolder() {
    files_.clear();
    std::filesystem::path folder{folderInput_};
    std::error_code error;
    if (!std::filesystem::is_directory(folder, error) || error) {
      status_ = "Not a readable folder: " + folderInput_;
      valid_ = false;
      return;
    }

    directoryPath_ = std::move(folder);
    std::filesystem::directory_iterator iterator{directoryPath_, error};
    const std::filesystem::directory_iterator end;
    while (!error && iterator != end) {
      const auto &entry = *iterator;
      if (entry.is_regular_file(error) && !error &&
          entry.path().extension() == ".arui")
        files_.push_back(entry.path());
      iterator.increment(error);
    }
    if (error) {
      files_.clear();
      status_ = "Could not scan " + directoryPath_.string();
      valid_ = false;
      return;
    }
    std::ranges::sort(
        files_, {}, [](const auto &path) { return path.filename().string(); });
  }

  void LoadFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      status_ = "Could not open " + path.string();
      valid_ = false;
      return;
    }

    source_.assign(std::istreambuf_iterator<char>{input},
                   std::istreambuf_iterator<char>{});
    sourcePath_ = path;
    std::error_code error;
    sourceWriteTime_ = std::filesystem::last_write_time(sourcePath_, error);
    if (error)
      sourceWriteTime_.reset();
    ParseAndSubmit();
  }

  void PollLiveReload() {
    if (!liveReload_ || sourcePath_.empty())
      return;
    std::error_code error;
    const auto writeTime = std::filesystem::last_write_time(sourcePath_, error);
    if (error) {
      status_ = "Live reload cannot access " + sourcePath_.string();
      valid_ = false;
      return;
    }
    if (!sourceWriteTime_ || writeTime != *sourceWriteTime_)
      LoadFile(sourcePath_);
  }

  void CreateRenderResources() {
    renderDevice_ = std::make_unique<ARUI::Render::OpenGLRenderDevice>(true);
    previewImage_ = renderDevice_->CreateImage(
        {.extent = {RenderExtent.x, RenderExtent.y, 1},
         .format = ARUI::Render::ImageFormat::R8G8B8A8_UNORM,
         .usage = static_cast<ARUI::Render::ImageUsage>(
                      ARUI::Render::ImageUsageFlags::ColorAttachment) |
                  static_cast<ARUI::Render::ImageUsage>(
                      ARUI::Render::ImageUsageFlags::Sampled)});
    previewView_ = renderDevice_->CreateImageView(
        {.image = previewImage_,
         .format = ARUI::Render::ImageFormat::R8G8B8A8_UNORM});

    standardPipeline_ =
        std::make_unique<ARUI::Render::StandardPipeline>(*renderDevice_);
  }

  void DestroyRenderResources() {
    if (!renderDevice_)
      return;
    renderDevice_->MakeCurrent();
    standardPipeline_.reset();
    renderDevice_->Destroy(previewView_);
    renderDevice_->Destroy(previewImage_);
    renderDevice_.reset();
  }

  void RenderPreview() {
    renderDevice_->MakeCurrent();
    ARUI::Render::Renderer renderer(
        *renderDevice_, standardPipeline_->Configuration(
                            {.colorAttachment = previewView_,
                             .clearColor = true,
                             .clearColorValue = {0.055F, 0.065F, 0.08F, 1.0F},
                             .extent = RenderExtent,
                             .offset = {0, 0}}));
    ARUI::Render::RenderGraph graph;
    renderer.BeginFrame();
    ARUI::Runtime::PaintContext paintContext{renderer};
    for (const NodeID root : runtime_.RootChildren()) {
      /*  const auto *surface = runtime_.Get(root);
       const float surfaceWidth =
           surface ? surface->style.width.As(ARUI::Language::LengthUnit::Meter)
                         .Value()
                   : 0.0F;
       const float surfaceHeight =
           surface ? surface->style.height.As(ARUI::Language::LengthUnit::Meter)
                         .Value()
                   : 0.0F;
       if (surfaceWidth <= 0.0F || surfaceHeight <= 0.0F)
         continue;
       const float pixelsPerMeter =
           std::min(static_cast<float>(RenderExtent.x) / surfaceWidth,
                    static_cast<float>(RenderExtent.y) / surfaceHeight);
       const glm::vec2 metersToNdc{
           2.0F * pixelsPerMeter / static_cast<float>(RenderExtent.x),
           2.0F * pixelsPerMeter / static_cast<float>(RenderExtent.y)};
       const glm::mat4 localToClip =
           glm::scale(glm::mat4{1.0F}, {metersToNdc.x, metersToNdc.y, 1.0F});
       ARUI::Runtime::PaintRuntimeSurface(
           paintContext, painters_, runtime_, root,
           {.localToClip = localToClip, .pixelsPerMeter = pixelsPerMeter}); */
      const auto *surface = runtime_.Get(root);
      const glm::vec2 surfaceSizeMeter{
          surface ? surface->style.width.As(ARUI::Language::LengthUnit::Meter)
                        .Value()
                  : 0.0F,
          surface ? surface->style.height.As(ARUI::Language::LengthUnit::Meter)
                        .Value()
                  : 0.0F};
      if (glm::any(glm::lessThanEqual(surfaceSizeMeter, glm::vec2{0.0F})))
        continue;
        

    }
    renderer.BuildRenderGraph(graph);
    graph.Compile();
    graph.Execute(*renderDevice_);
    renderer.EndFrame();
  }

  void ParseAndSubmit() {
    const auto parsed = ARUI::Language::ParseMarkup(source_);
    if (!parsed) {
      const auto &diagnostic = parsed.diagnostics.front();
      status_ = "Line " + std::to_string(diagnostic.line) + ", column " +
                std::to_string(diagnostic.column) + ": " + diagnostic.message;
      valid_ = false;
      return;
    }

    auto transaction = runtime_.BeginTransaction();
    for (const NodeID root : runtime_.RootChildren())
      transaction.Remove(root);
    for (const auto &surface : parsed.value.surfaces)
      transaction.InsertTree(runtime_.Root(), surface);
    transaction.Commit();
    status_ = "Runtime revision " + std::to_string(runtime_.Revision());
    valid_ = true;
  }

  void Draw() {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Markup Viewer", nullptr, flags);

    const float editorWidth =
        std::max(340.0F, ImGui::GetContentRegionAvail().x * 0.36F);
    ImGui::BeginChild("Source", {editorWidth, 0.0F}, true);
    ImGui::TextUnformatted("ARUI XML");
    ImGui::SetNextItemWidth(-80.0F);
    const bool folderSubmitted = ImGui::InputText(
        "##Folder", &folderInput_, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Scan") || folderSubmitted)
      ScanFolder();
    if (files_.empty()) {
      ImGui::TextDisabled("Enter or drop a folder containing .arui files");
    } else {
      const bool filesVisible =
          ImGui::BeginChild("Files", {-1.0F, 125.0F}, true);
      if (filesVisible) {
        for (const auto &file : files_) {
          const bool selected = file == sourcePath_;
          if (ImGui::Selectable(file.filename().string().c_str(), selected))
            LoadFile(file);
        }
      }
      ImGui::EndChild();
    }
    if (sourcePath_.empty())
      ImGui::TextDisabled("Drop an ARUI file anywhere in this window");
    else
      ImGui::TextDisabled("%s", sourcePath_.string().c_str());
    ImGui::BeginDisabled(sourcePath_.empty());
    ImGui::Checkbox("Live reload", &liveReload_);
    ImGui::EndDisabled();
    ImGui::Separator();
    const float statusHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0F;
    if (ImGui::InputTextMultiline("##Markup", &source_, {-1.0F, -statusHeight},
                                  ImGuiInputTextFlags_AllowTabInput))
      ParseAndSubmit();
    ImGui::PushStyleColor(ImGuiCol_Text,
                          valid_ ? ImVec4{0.35F, 0.8F, 0.5F, 1.0F}
                                 : ImVec4{1.0F, 0.38F, 0.32F, 1.0F});
    ImGui::TextWrapped("%s", status_.c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Preview", {0.0F, 0.0F}, true,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 available = ImGui::GetContentRegionAvail();
    constexpr float aspect =
        static_cast<float>(RenderExtent.x) / static_cast<float>(RenderExtent.y);
    ImVec2 size = available;
    if (size.y > 0.0F && size.x / size.y > aspect)
      size.x = size.y * aspect;
    else
      size.y = size.x / aspect;
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos({cursor.x + (available.x - size.x) * 0.5F,
                         cursor.y + (available.y - size.y) * 0.5F});
    const GLuint texture = renderDevice_->GetGLTextureView(previewView_).id;
    ImGui::Image(static_cast<ImTextureID>(texture), size, {0.0F, 1.0F},
                 {1.0F, 0.0F});
    ImGui::EndChild();
    ImGui::End();
  }

  GLFWwindow *window_{};
  RuntimeTree runtime_;
  ARUI::Runtime::PainterRegistry painters_;
  std::string source_;
  std::string status_;
  std::filesystem::path sourcePath_;
  std::filesystem::path directoryPath_;
  std::string folderInput_;
  std::vector<std::filesystem::path> files_;
  std::optional<std::filesystem::file_time_type> sourceWriteTime_;
  bool valid_{};
  bool liveReload_{};

  std::unique_ptr<ARUI::Render::OpenGLRenderDevice> renderDevice_;
  ARUI::Render::ImageHandle previewImage_;
  ARUI::Render::ImageViewHandle previewView_;
  std::unique_ptr<ARUI::Render::StandardPipeline> standardPipeline_;
};

} // namespace

int main(int argc, char **argv) {
  try {
    MarkupViewer viewer(argc > 1 ? std::filesystem::path{argv[1]}
                                 : std::filesystem::path{});
    return viewer.Run();
  } catch (const std::exception &error) {
    std::cerr << "Markup viewer failed: " << error.what() << '\n';
    return 1;
  }
}
