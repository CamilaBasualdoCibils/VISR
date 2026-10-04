find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(AruiManualPresenter src/main.cpp)
target_link_libraries(AruiManualPresenter PRIVATE
    ARUI::Language
    ARUI::Presentation
    glfw
    imgui::imgui
    OpenGL::GL)
arui_enable_runtime_instrumentation(AruiManualPresenter)
