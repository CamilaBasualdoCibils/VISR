add_executable(AruiDesktop main.cpp AruiDesktop.cpp)
target_link_libraries(AruiDesktop PRIVATE
    ARUI::ServerManager
    ARUI::RenderOpenGL
    ARUI::OpenXR
    ARUI::RenderOpenGLOpenXR
    Boost::program_options)
arui_enable_runtime_instrumentation(AruiDesktop)
