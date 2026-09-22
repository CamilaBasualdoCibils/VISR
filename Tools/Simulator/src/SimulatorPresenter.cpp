#include "ARUI/Tools/Simulator/SimulatorPresenter.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
void APIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id,
                                  GLenum severity, GLsizei length,
                                  const GLchar *message,
                                  const void *userParam) {
  static auto logger = spdlog::stdout_color_mt("OpenGL");

  auto logLevel = spdlog::level::info;

  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    logLevel = spdlog::level::err;

    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    logLevel = spdlog::level::warn;
    break;
  case GL_DEBUG_SEVERITY_LOW:
    logLevel = spdlog::level::info;
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    logLevel = spdlog::level::debug;
    break;
  }
  logger->log(logLevel, "OpenGL Debug Message [{}]: {}", id, message);
}
void glfw_error_callback(int error, const char *description) {
  static auto logger = spdlog::stdout_color_mt("GLFW");
  logger->error("[{}]: {}", error, description);
}
void ARUI::Tools::Simulator::SimulatorPresenter::BeginFrame() {
  glfwPollEvents();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}
void ARUI::Tools::Simulator::SimulatorPresenter::Present(const RenderView &view,
                                            RenderTargetHandle target) {
  // window with toolbar and fullscreen image

  if (ImGui::Begin(view.Name.c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open")) {
          // Handle open action
        }
        if (ImGui::MenuItem("Save")) {
          // Handle save action
        }
        if (ImGui::MenuItem("Exit")) {
          // Handle exit action
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }
  }
  ImGui::End();
}
void ARUI::Tools::Simulator::SimulatorPresenter::EndFrame() {
  DrawSimulator();
  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
}

ARUI::Tools::Simulator::SimulatorPresenter::SimulatorPresenter() {
  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("GLEW failed to init");
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required for OSX
  glfwWindowHint(GLFW_RESIZABLE, true);
  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  window = glfwCreateWindow(1920, 1080, "Simulator", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  if (glewInit() != GLEW_OK) {
    throw std::runtime_error("GLEW failed to init");
  }
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensures messages are synchronous
  glDebugMessageCallback(openglDebugCallback, nullptr);
  const GLubyte *version = glGetString(GL_VERSION);
  logger->info("OpenGL Version Supported: {}",
               reinterpret_cast<const char *>(version));

  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 150");
  ImGui::StyleColorsDark();
  ImPlot::CreateContext();
  auto &io = ImGui::GetIO();
  glm::vec2 content_scale;
  glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &content_scale.x,
                             &content_scale.y);
  ImGui::GetStyle().ScaleAllSizes(content_scale.x);
  ImGui::GetIO().FontGlobalScale = content_scale.x;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;              // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // E
}
ARUI::Tools::Simulator::SimulatorPresenter::~SimulatorPresenter() {}
void ARUI::Tools::Simulator::SimulatorPresenter::DrawSimulator() { ImGui::ShowDemoWindow(); }
