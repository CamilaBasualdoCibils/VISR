add_library(ARUIRender INTERFACE)
add_library(ARUI::Render ALIAS ARUIRender)
target_include_directories(ARUIRender INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIRender
    INTERFACE
    ARUI::Core
    fg::FrameGraph
    Vulkan::Vulkan
    ARUI_OpenGLShaders)
add_subdirectory(Backends/OpenGL)
add_subdirectory(Backends/Vulkan)
