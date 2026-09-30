add_executable(AruiServer main.cpp AruiDesktop.cpp)
target_link_libraries(AruiServer PRIVATE
    ARUI::ServerManager
    ARUI::Simulator
    ARUI::RenderOpenGL
    ARUI::RenderVulkan
    Boost::program_options)
arui_enable_runtime_instrumentation(AruiServer)
