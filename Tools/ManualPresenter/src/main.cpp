#include "ARUI/Language/Parser.hpp"
#include "ARUI/Presentation/RpcPresentationController.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

constexpr const char *kDefaultStack = R"(<stack>
  <panel>
    <text>Hello from ARUI Manual Presenter</text>
  </panel>
</stack>)";

class ManualPresenter {
public:
  ManualPresenter() : stackSource_(kDefaultStack) {
    if (glfwInit() != GLFW_TRUE)
      throw std::runtime_error("GLFW failed to initialize");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ =
        glfwCreateWindow(900, 700, "ARUI Manual Presenter", nullptr, nullptr);
    if (!window_)
      throw std::runtime_error("GLFW failed to create the window");
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
  }

  ~ManualPresenter() {
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
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      Draw();
      ImGui::Render();
      int width = 0, height = 0;
      glfwGetFramebufferSize(window_, &width, &height);
      glViewport(0, 0, width, height);
      glClearColor(0.045F, 0.05F, 0.065F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      glfwSwapBuffers(window_);
    }
    return 0;
  }

private:
  void Draw() {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("ARUI Manual Presenter", nullptr, flags);
    ImGui::TextUnformatted("ARUI Manual Presenter");
    ImGui::TextDisabled(
        "Enter a <stack> and push it to the active ARUI surface.");
    ImGui::Separator();

    ImGui::SetNextItemWidth(320.0F);
    ImGui::InputText("Server", &host_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0F);
    ImGui::InputInt("Port", &port_);
    port_ = std::clamp(port_, 1, 65535);

    ImGui::TextUnformatted("Stack markup");
    ImGui::InputTextMultiline("##stack", &stackSource_, {-1.0F, -95.0F},
                              ImGuiInputTextFlags_AllowTabInput);
    if (ImGui::Button("Present", {120.0F, 0.0F}))
      Present();
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
      stackSource_ = kDefaultStack;
    ImGui::SameLine();
    if (statusError_)
      ImGui::TextColored({1.0F, 0.35F, 0.35F, 1.0F}, "%s", status_.c_str());
    else
      ImGui::TextColored({0.35F, 1.0F, 0.55F, 1.0F}, "%s", status_.c_str());
    ImGui::End();
  }

  void Present() {
    const auto parsed = ARUI::Language::ParseMarkup(stackSource_);
    if (!parsed) {
      const auto &diagnostic = parsed.diagnostics.front();
      status_ = "Line " + std::to_string(diagnostic.line) + ", column " +
                std::to_string(diagnostic.column) + ": " + diagnostic.message;
      statusError_ = true;
      return;
    }
    try {
      ARUI::Presentation::RpcPresentationController controller{
          host_, static_cast<std::uint16_t>(port_)};
      controller.SetActiveTree(parsed.value.surfaces.front());
      status_ = "Presented to " + host_ + ":" + std::to_string(port_);
      statusError_ = false;
    } catch (const std::exception &error) {
      status_ = error.what();
      statusError_ = true;
    }
  }

  GLFWwindow *window_{};
  std::string host_{"127.0.0.1"};
  int port_{4243};
  std::string stackSource_;
  std::string status_{"Ready"};
  bool statusError_{false};
};

} // namespace

int main() {
  try {
    return ManualPresenter{}.Run();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "arui-manual-presenter: %s\n", error.what());
    return 1;
  }
}
