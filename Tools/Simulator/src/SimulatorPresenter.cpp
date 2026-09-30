#include "ARUI/Tools/Simulator/SimulatorPresenter.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace {
void GLFWErrorCallback(int error, const char *description) {
  static auto logger = spdlog::stdout_color_mt("GLFW");
  logger->error("[{}]: {}", error, description);
}
} // namespace

namespace ARUI::Tools::Simulator {

SimulatorPresenter::SimulatorPresenter()
    : SimulatorPresenter(std::make_shared<SimulatorXRTracker>(), nullptr) {
  views_ = std::make_shared<SimulatorViewProvider>(tracker_);
}

SimulatorPresenter::SimulatorPresenter(
    std::shared_ptr<SimulatorXRTracker> tracker,
    std::shared_ptr<SimulatorViewProvider> views)
    : tracker_(std::move(tracker)), views_(std::move(views)) {
  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  glfwSetErrorCallback(GLFWErrorCallback);
  if (glfwInit() != GLFW_TRUE)
    throw std::runtime_error("GLFW failed to initialize");

  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  window = glfwCreateWindow(1920, 1080, "ARUI Simulator", nullptr, nullptr);
  if (!window)
    throw std::runtime_error("GLFW failed to create the simulator window");
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 450");
  ImGui::StyleColorsDark();
  auto &style = ImGui::GetStyle();
  style.WindowRounding = 0.0F;
  style.ChildRounding = 0.0F;
  style.FrameRounding = 2.0F;
  style.TabRounding = 2.0F;
  style.WindowBorderSize = 1.0F;
  style.Colors[ImGuiCol_WindowBg] = {0.055F, 0.065F, 0.08F, 1.0F};
  style.Colors[ImGuiCol_TitleBg] = {0.035F, 0.12F, 0.16F, 1.0F};
  style.Colors[ImGuiCol_TitleBgActive] = {0.04F, 0.2F, 0.26F, 1.0F};
  style.Colors[ImGuiCol_TabSelected] = {0.04F, 0.24F, 0.31F, 1.0F};
  auto &io = ImGui::GetIO();
  glm::vec2 content_scale;
  glfwGetWindowContentScale(window, &content_scale.x, &content_scale.y);
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.FontGlobalScale = content_scale.y;
}

SimulatorPresenter::~SimulatorPresenter() {
  ImPlot::DestroyContext();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  if (window)
    glfwDestroyWindow(window);
  glfwTerminate();
}

std::vector<std::string>
SimulatorPresenter::GetRequiredVulkanExtensions() const {
  uint32_t count = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&count);
  std::vector<std::string> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    result.emplace_back(extensions[i]);
  return result;
}

void SimulatorPresenter::BeginFrame() {
  glfwPollEvents();
  glfwMakeContextCurrent(window);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  presentedViewCount_ = 0;
  DrawEditorShell();
  DrawTrackingInspector();
}

void SimulatorPresenter::DrawEditorShell() {
  const ImGuiID dockspace =
      ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
  if (!dockLayoutInitialized_) {
    dockLayoutInitialized_ = true;
    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace,
                                  ImGui::GetMainViewport()->WorkSize);
    ImGuiID center = dockspace;
    ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22F,
                                               nullptr, &center);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.22F,
                                                 nullptr, &center);
    ImGui::DockBuilderDockWindow("Scene", left);
    ImGui::DockBuilderDockWindow("Tracking & Poses", left);
    ImGui::DockBuilderDockWindow("Viewport", center);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderFinish(dockspace);
  }
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit"))
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Tracking Inspector", nullptr, true, false);
      ImGui::EndMenu();
    }
    ImGui::TextDisabled("ARUI Simulator");
    ImGui::EndMainMenuBar();
  }
}

void SimulatorPresenter::DrawPoseEditor(const char *label, Pose &pose) {
  if (!ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
    return;
  ImGui::PushID(label);
  ImGui::DragFloat3("Position", &pose.position.x, 0.01F, -100.0F, 100.0F,
                    "%.3f m");
  glm::vec3 rotation = glm::degrees(glm::eulerAngles(pose.orientation));
  if (ImGui::DragFloat3("Rotation", &rotation.x, 0.25F, -180.0F, 180.0F,
                        "%.1f deg"))
    pose.orientation = glm::quat(glm::radians(rotation));
  if (ImGui::Button("Reset"))
    pose = {};
  ImGui::PopID();
  ImGui::TreePop();
}

void SimulatorPresenter::DrawTrackedPoseEditor(const char *label,
                                               TrackedPose &trackedPose) {
  DrawPoseEditor(label, trackedPose.pose);
  ImGui::PushID(label);
  ImGui::Indent();
  ImGui::Checkbox("Position valid", &trackedPose.positionValid);
  ImGui::SameLine();
  ImGui::Checkbox("Position tracked", &trackedPose.positionTracked);
  ImGui::Checkbox("Orientation valid", &trackedPose.orientationValid);
  ImGui::SameLine();
  ImGui::Checkbox("Orientation tracked", &trackedPose.orientationTracked);
  ImGui::Unindent();
  ImGui::PopID();
}

void SimulatorPresenter::DrawTrackingInspector() {
  ImGui::SetNextWindowSize({350.0F, 600.0F}, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Tracking & Poses")) {
    ImGui::TextDisabled("Values update all simulated views immediately.");
    ImGui::Separator();
    DrawTrackedPoseEditor("Head", tracker_->Head());
    DrawTrackedPoseEditor("Left Hand", tracker_->LeftHand());
    DrawTrackedPoseEditor("Right Hand", tracker_->RightHand());
    ImGui::SeparatorText("Cameras");
    DrawPoseEditor("Debug Camera", views_->DebugCamera());
  }
  ImGui::End();
}

void SimulatorPresenter::SetRenderDevice(
    std::shared_ptr<Render::OpenGLRenderDevice> renderDevice) {
  renderDevice_ = std::move(renderDevice);
}

void SimulatorPresenter::Present(const RenderView &view,
                                 Render::ImageViewHandle image) {
  if (presentedViewCount_ < presentedViews_.size())
    presentedViews_[presentedViewCount_++] = {view, image};
}

void SimulatorPresenter::DrawScenePanel() {
  if (ImGui::Begin("Scene")) {
    ImGui::SeparatorText("Cameras");
    for (std::size_t i = 0; i < presentedViewCount_; ++i) {
      const bool selected = selectedView_ == static_cast<int>(i);
      if (ImGui::Selectable(presentedViews_[i].view.Name.c_str(), selected))
        selectedView_ = static_cast<int>(i);
    }
    ImGui::SeparatorText("Trackers");
    ImGui::BulletText("Head");
    ImGui::BulletText("Left Hand");
    ImGui::BulletText("Right Hand");
  }
  ImGui::End();
}

void SimulatorPresenter::DrawViewport() {
  if (presentedViewCount_ == 0)
    selectedView_ = 0;
  else if (selectedView_ >= static_cast<int>(presentedViewCount_))
    selectedView_ = static_cast<int>(presentedViewCount_ - 1);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0F, 0.0F});
  const bool visible =
      ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_MenuBar);
  ImGui::PopStyleVar();
  if (visible) {
    if (ImGui::BeginMenuBar()) {
      ImGui::TextDisabled("View");
      ImGui::SetNextItemWidth(180.0F);
      const char *preview =
          presentedViewCount_ == 0
              ? "No view"
              : presentedViews_[selectedView_].view.Name.c_str();
      if (ImGui::BeginCombo("##ActiveView", preview)) {
        for (std::size_t i = 0; i < presentedViewCount_; ++i) {
          const bool selected = selectedView_ == static_cast<int>(i);
          if (ImGui::Selectable(presentedViews_[i].view.Name.c_str(), selected))
            selectedView_ = static_cast<int>(i);
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::Separator();
      ImGui::TextDisabled("800 x 600");
      ImGui::EndMenuBar();
    }

    if (presentedViewCount_ != 0 && renderDevice_) {
      const auto &presented = presentedViews_[selectedView_];
      const ImVec2 available = ImGui::GetContentRegionAvail();
      const float aspect = static_cast<float>(presented.view.viewportSize.x) /
                           static_cast<float>(presented.view.viewportSize.y);
      ImVec2 size = available;
      if (size.y > 0.0F && size.x / size.y > aspect)
        size.x = size.y * aspect;
      else if (aspect > 0.0F)
        size.y = size.x / aspect;
      const ImVec2 cursor = ImGui::GetCursorPos();
      ImGui::SetCursorPos({cursor.x + (available.x - size.x) * 0.5F,
                           cursor.y + (available.y - size.y) * 0.5F});
      const auto texture = renderDevice_->GetGLTextureView(presented.image).id;
      ImGui::Image(static_cast<ImTextureID>(texture), size, {0.0F, 1.0F},
                   {1.0F, 0.0F});
    } else {
      ImGui::SetCursorPos({24.0F, 48.0F});
      ImGui::TextDisabled("No rendered view available");
    }
  }
  ImGui::End();
}

void SimulatorPresenter::DrawConsole() {
  if (ImGui::Begin("Console")) {
    ImGui::TextDisabled("ARUI Simulator ready");
    ImGui::Text("Three simulated views are rendering through OpenGL/EGL.");
  }
  ImGui::End();
}

void SimulatorPresenter::EndFrame() {
  DrawScenePanel();
  DrawViewport();
  DrawConsole();
  glfwMakeContextCurrent(window);
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup);
  }
  glfwSwapBuffers(window);
}

bool SimulatorPresenter::ShouldClose() const {
  return !window || glfwWindowShouldClose(window);
}

} // namespace ARUI::Tools::Simulator
