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
ARUI::Language::LNode MakeDefaultDocument() {
  using namespace ARUI;
  Language::Style surfaceStyle;
  surfaceStyle.width = Language::Length{1.2, Language::LengthUnit::Meter};
  surfaceStyle.height = Language::Length{0.7, Language::LengthUnit::Meter};
  surfaceStyle.padding = Language::Length{0.08, Language::LengthUnit::Meter};
  surfaceStyle.zOffset = Language::Length{-2.0, Language::LengthUnit::Meter};

  Language::Style panelStyle;
  panelStyle.padding = Language::Length{0.06, Language::LengthUnit::Meter};

  return Language::LSurface(
      {Language::LPanel({Language::LText("Hello world")}, {},
                        panelStyle)},
      {.anchor = "head"}, surfaceStyle);
}

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
    : tracker_(std::move(tracker)), views_(std::move(views)),
      draftDocuments_{MakeDefaultDocument()} {
  submittedDocuments_ = draftDocuments_;
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
  DrawDocumentEditor();
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
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25F,
                                                nullptr, &center);
    ImGui::DockBuilderDockWindow("Scene", left);
    ImGui::DockBuilderDockWindow("Tracking & Poses", left);
    ImGui::DockBuilderDockWindow("Document", right);
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

bool SimulatorPresenter::DrawLanguageNodeEditor(Language::LNode &node,
                                                bool root) {
  static constexpr std::array types = {
      Language::LNodeType::Surface, Language::LNodeType::Group,
      Language::LNodeType::Row,     Language::LNodeType::Column,
      Language::LNodeType::Stack,   Language::LNodeType::Text,
      Language::LNodeType::Button,  Language::LNodeType::Panel};
  static constexpr std::array names = {"Surface", "Group", "Row",    "Column",
                                       "Stack",   "Text",  "Button", "Panel"};
  const auto typeIndex = [&] {
    for (std::size_t i = 0; i < types.size(); ++i)
      if (types[i] == node.type)
        return i;
    return std::size_t{0};
  }();

  ImGui::PushID(&node);
  const bool open = ImGui::TreeNodeEx("Node",
                                      ImGuiTreeNodeFlags_DefaultOpen |
                                          ImGuiTreeNodeFlags_SpanAvailWidth,
                                      "%s", names[typeIndex]);
  bool remove = false;
  if (open) {
    if (root) {
      ImGui::TextDisabled("Type: Surface (required root)");
    } else {
      ImGui::SetNextItemWidth(-1.0F);
      if (ImGui::BeginCombo("Type", names[typeIndex])) {
        for (std::size_t i = 0; i < types.size(); ++i) {
          const bool selected = node.type == types[i];
          if (ImGui::Selectable(names[i], selected) && !selected) {
            node.type = types[i];
            documentChangedThisFrame_ = true;
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }

    if (node.type == Language::LNodeType::Text) {
      char buffer[256]{};
      if (const auto *text = node.GetAttribute<std::string>("text"))
        std::snprintf(buffer, sizeof(buffer), "%s", text->c_str());
      if (ImGui::InputText("Text", buffer, sizeof(buffer))) {
        node.SetAttribute("text", std::string{buffer});
        documentChangedThisFrame_ = true;
      }
    }

    auto editStringAttribute = [&](const char *label, const char *attribute) {
      char buffer[256]{};
      if (const auto *value = node.GetAttribute<std::string>(attribute))
        std::snprintf(buffer, sizeof(buffer), "%s", value->c_str());
      if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        node.SetAttribute(attribute, std::string{buffer});
        documentChangedThisFrame_ = true;
      }
    };
    if (node.type == Language::LNodeType::Surface) {
      static constexpr std::array surfaceTypes = {
          Language::SurfaceType::Plane, Language::SurfaceType::Cylinder,
          Language::SurfaceType::Sphere, Language::SurfaceType::Parametric,
          Language::SurfaceType::Mesh};
      static constexpr std::array surfaceTypeNames = {
          "Rectangle", "Cylinder", "Sphere", "Parametric", "Mesh"};
      std::size_t surfaceTypeIndex = 0;
      for (; surfaceTypeIndex < surfaceTypes.size(); ++surfaceTypeIndex)
        if (surfaceTypes[surfaceTypeIndex] == node.surfaceType)
          break;
      ImGui::SetNextItemWidth(-1.0F);
      if (ImGui::BeginCombo("Surface Type", surfaceTypeNames[surfaceTypeIndex])) {
        for (std::size_t i = 0; i < surfaceTypes.size(); ++i) {
          const bool selected = node.surfaceType == surfaceTypes[i];
          if (ImGui::Selectable(surfaceTypeNames[i], selected) && !selected) {
            node.surfaceType = surfaceTypes[i];
            documentChangedThisFrame_ = true;
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      static constexpr std::array anchors = {"world", "head", "left_hand",
                                              "right_hand"};
      const auto *storedAnchor = node.GetAttribute<std::string>("anchor");
      const char *anchor = storedAnchor ? storedAnchor->c_str() : "world";
      ImGui::SetNextItemWidth(-1.0F);
      if (ImGui::BeginCombo("Anchor", anchor)) {
        for (const char *candidate : anchors) {
          const bool selected = std::string_view{anchor} == candidate;
          if (ImGui::Selectable(candidate, selected) && !selected) {
            node.SetAttribute("anchor", std::string{candidate});
            documentChangedThisFrame_ = true;
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }
    if (node.type == Language::LNodeType::Panel ||
        node.type == Language::LNodeType::Group)
      editStringAttribute("Name", "name");

    auto beginStyleRow = [](const char *label) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(label);
      ImGui::TableSetColumnIndex(1);
      ImGui::PushID(label);
    };
    struct UnitChoice {
      Language::LengthUnit unit;
      const char *label;
    };
    static constexpr std::array unitChoices = {
        UnitChoice{Language::LengthUnit::Auto, "Auto"},
        UnitChoice{Language::LengthUnit::Pixel, "px"},
        UnitChoice{Language::LengthUnit::Millimeter, "mm"},
        UnitChoice{Language::LengthUnit::Centimeter, "cm"},
        UnitChoice{Language::LengthUnit::Meter, "m"},
        UnitChoice{Language::LengthUnit::Percent, "%"}};

    auto unitLabel = [](Language::LengthUnit unit) {
      for (const UnitChoice &choice : unitChoices)
        if (choice.unit == unit)
          return choice.label;
      return "Auto";
    };
    auto drawLengthValue = [this, &unitLabel](Language::Length &value,
                                               bool allowNegative) {
      ImGui::SetNextItemWidth(76.0F);
      if (ImGui::BeginCombo("##Unit", unitLabel(value.unit))) {
        for (const UnitChoice &choice : unitChoices) {
          const bool selected = value.unit == choice.unit;
          if (ImGui::Selectable(choice.label, selected) && !selected) {
            const bool wasAuto = value.IsAuto();
            value.unit = choice.unit;
            if (wasAuto && !value.IsAuto() && value.value == 0.0)
              value.value =
                  value.unit == Language::LengthUnit::Percent ? 100.0 : 1.0;
            if (value.IsAuto())
              value.value = 0.0;
            documentChangedThisFrame_ = true;
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      ImGui::SameLine();
      float numericValue = value.unit == Language::LengthUnit::Percent
                               ? static_cast<float>(value.value / 100.0)
                               : static_cast<float>(value.value);
      ImGui::BeginDisabled(value.IsAuto());
      ImGui::SetNextItemWidth(-1.0F);
      const float minimum = allowNegative ? -10000.0F : 0.0F;
      if (ImGui::DragFloat("##Value", &numericValue, 0.01F, minimum,
                           10000.0F, "%.2f")) {
        value.value = value.unit == Language::LengthUnit::Percent
                          ? numericValue * 100.0
                          : numericValue;
        documentChangedThisFrame_ = true;
      }
      ImGui::EndDisabled();
    };
    auto editLength = [this, &beginStyleRow, &drawLengthValue, &unitLabel](
                          const char *label,
                          std::optional<Language::Length> &value) {
      beginStyleRow(label);
      const char *preview = value ? unitLabel(value->unit) : "Unset";
      ImGui::SetNextItemWidth(76.0F);
      if (ImGui::BeginCombo("##Presence", preview)) {
        const bool unset = !value;
        if (ImGui::Selectable("Unset", unset) && !unset) {
          value.reset();
          documentChangedThisFrame_ = true;
        }
        for (const UnitChoice &choice : unitChoices) {
          const bool selected = value && value->unit == choice.unit;
          if (ImGui::Selectable(choice.label, selected) && !selected) {
            const double initial = choice.unit == Language::LengthUnit::Auto
                                       ? 0.0
                                   : choice.unit == Language::LengthUnit::Percent
                                       ? 100.0
                                       : 1.0;
            value = Language::Length{initial, choice.unit};
            documentChangedThisFrame_ = true;
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      if (value) {
        ImGui::SameLine();
        float numericValue = value->unit == Language::LengthUnit::Percent
                                 ? static_cast<float>(value->value / 100.0)
                                 : static_cast<float>(value->value);
        ImGui::BeginDisabled(value->IsAuto());
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::DragFloat("##Value", &numericValue, 0.01F, -10000.0F,
                             10000.0F, "%.2f")) {
          value->value = value->unit == Language::LengthUnit::Percent
                             ? numericValue * 100.0
                             : numericValue;
          documentChangedThisFrame_ = true;
        }
        ImGui::EndDisabled();
      }
      ImGui::PopID();
    };
    auto editSize = [&beginStyleRow, &drawLengthValue](
                        const char *label, Language::Length &size) {
      beginStyleRow(label);
      drawLengthValue(size, false);
      ImGui::PopID();
    };

    ImGui::SeparatorText("Style");
    if (ImGui::BeginTable("StyleFields", 2,
                          ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed,
                              86.0F);
      ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
      editSize("Width", node.style.width);
      editSize("Height", node.style.height);
      editSize("Min Width", node.style.minWidth);
      editSize("Min Height", node.style.minHeight);
      editSize("Max Width", node.style.maxWidth);
      editSize("Max Height", node.style.maxHeight);
      editLength("Margin", node.style.margin);
      editLength("Padding", node.style.padding);
      editLength("Gap", node.style.gap);
      editLength("X Offset", node.style.xOffset);
      editLength("Y Offset", node.style.yOffset);
      editLength("Z Offset", node.style.zOffset);
      ImGui::EndTable();
    }

    if (ImGui::SmallButton("+ Child")) {
      node.children.push_back(Language::LPanel());
      documentChangedThisFrame_ = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Text")) {
      node.children.push_back(Language::LText("Text"));
      documentChangedThisFrame_ = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(root ? "Remove Surface" : "Remove")) {
      remove = true;
      documentChangedThisFrame_ = true;
    }

    for (std::size_t i = 0; i < node.children.size();) {
      if (DrawLanguageNodeEditor(node.children[i], false))
        node.children.erase(node.children.begin() +
                            static_cast<std::ptrdiff_t>(i));
      else
        ++i;
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
  return remove;
}

void SimulatorPresenter::DrawDocumentEditor() {
  documentChangedThisFrame_ = false;
  if (ImGui::Begin("Document")) {
    ImGui::TextDisabled("Every document root must be a Surface");
    ImGui::Checkbox("Auto-submit changes", &autoSubmit_);
    if (ImGui::Button("+ Surface", {-1.0F, 0.0F})) {
      draftDocuments_.push_back(Language::LSurface());
      documentChangedThisFrame_ = true;
    }
    ImGui::Separator();

    for (std::size_t i = 0; i < draftDocuments_.size();) {
      if (DrawLanguageNodeEditor(draftDocuments_[i], true))
        draftDocuments_.erase(draftDocuments_.begin() +
                              static_cast<std::ptrdiff_t>(i));
      else
        ++i;
    }

    ImGui::Separator();
    if (ImGui::Button("Submit to Runtime", {-1.0F, 0.0F}))
      submittedDocuments_ = draftDocuments_;
    if (autoSubmit_ && documentChangedThisFrame_)
      submittedDocuments_ = draftDocuments_;
    ImGui::TextDisabled("Surfaces: %zu | Runtime revision: %llu",
                        draftDocuments_.size(),
                        static_cast<unsigned long long>(runtimeRevision_));
  }
  ImGui::End();
}

std::optional<std::vector<Language::LNode>>
SimulatorPresenter::TakeSubmittedDocument() {
  auto submitted = std::move(submittedDocuments_);
  submittedDocuments_.reset();
  return submitted;
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
