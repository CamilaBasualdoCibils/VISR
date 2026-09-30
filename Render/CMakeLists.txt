add_library(ARUIRender STATIC
    src/Painter.cpp
    src/Renderer.cpp
    src/RenderGraph.cpp)
add_library(ARUI::Render ALIAS ARUIRender)
target_include_directories(ARUIRender PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIRender
    PUBLIC
    ARUI::Core
    ARUI::Language
    fg::FrameGraph
    Vulkan::Vulkan
    ARUI_OpenGLShaders)
target_compile_features(ARUIRender PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()

add_subdirectory(Backends/OpenGL)
add_subdirectory(Backends/Vulkan)
