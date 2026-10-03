add_library(ARUIRender STATIC
    src/Font.cpp
    src/Painter.cpp
    src/Renderer.cpp
    src/RenderGraph.cpp
    src/RuntimePainter.cpp
    src/StandardPipeline.cpp)
add_library(ARUI::Render ALIAS ARUIRender)
target_include_directories(ARUIRender PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIRender
    PUBLIC
    ARUI::Core
    ARUI::Language
    ARUI::Runtime
    fg::FrameGraph
    Vulkan::Vulkan
    ARUI_OpenGLShaders
    Freetype::Freetype)
find_file(ARUI_NOTO_SANS_FONT
    NAMES NotoSans-Regular.ttf
    PATHS /usr/share/fonts/noto /usr/share/fonts/truetype/noto
    REQUIRED)
target_compile_definitions(ARUIRender PRIVATE
    ARUI_DEFAULT_FONT_PATH="${ARUI_NOTO_SANS_FONT}")
target_compile_features(ARUIRender PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()

add_subdirectory(Backends/OpenGL)
add_subdirectory(Backends/Vulkan)
