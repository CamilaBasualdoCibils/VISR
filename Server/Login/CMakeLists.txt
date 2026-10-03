add_executable(AruiLogin main.cpp)
target_link_libraries(AruiLogin PRIVATE
    ARUI::RenderOpenGL
    ARUI::OpenXR
    ARUI::RenderOpenGLOpenXR)
arui_enable_runtime_instrumentation(AruiLogin)
