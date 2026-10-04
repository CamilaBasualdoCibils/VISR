add_library(VISRRender STATIC
    src/Font.cpp
    src/Renderer.cpp
    src/RenderGraph.cpp
    src/StandardPipeline.cpp)
add_library(VISR::Render ALIAS VISRRender)
target_include_directories(VISRRender PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRRender
    PUBLIC
    VISR::Core
    VISR::Language
    VISR::Runtime
    fg::FrameGraph
    Vulkan::Vulkan
    VISR_OpenGLShaders
    Freetype::Freetype)
find_file(VISR_NOTO_SANS_FONT
    NAMES NotoSans-Regular.ttf
    PATHS /usr/share/fonts/noto /usr/share/fonts/truetype/noto
    REQUIRED)
target_compile_definitions(VISRRender PRIVATE
    VISR_DEFAULT_FONT_PATH="${VISR_NOTO_SANS_FONT}")
target_compile_features(VISRRender PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()

add_subdirectory(Backends/OpenGL)
add_subdirectory(Backends/Vulkan)
