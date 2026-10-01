add_executable(AruiMarkupViewer src/main.cpp)
target_link_libraries(AruiMarkupViewer PRIVATE
    ARUI::Runtime
    ARUI::RenderOpenGL
    imgui::imgui
    glfw
    OpenGL::GL)
arui_enable_runtime_instrumentation(AruiMarkupViewer)
