add_executable(AruiServer main.cpp AruiServer.cpp)
target_link_libraries(AruiServer PRIVATE
    ARUI::PresentationServer
    ARUI::RenderOpenGL
    ARUI::OpenXR
    ARUI::RenderOpenGLOpenXR
    Boost::program_options)
arui_enable_runtime_instrumentation(AruiServer)
