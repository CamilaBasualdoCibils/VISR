add_executable(VisrMarkupViewer src/main.cpp)
target_link_libraries(VisrMarkupViewer PRIVATE
    VISR::Runtime
    VISR::RenderOpenGL
    imgui::imgui
    glfw
    OpenGL::GL)
visr_enable_runtime_instrumentation(VisrMarkupViewer)
