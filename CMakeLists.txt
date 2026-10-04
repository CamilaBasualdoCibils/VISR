cmake_minimum_required(VERSION 3.10.0)
project(ARUI VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ARUI_ENABLE_TRACY "Enable Tracy profiling instead of mimalloc" OFF)

include(cmake/Instrumentation.cmake)
include(cmake/FindDeps.cmake)
include(CTest)
include(cmake/CompileShaders.cmake)
enable_testing()
add_spirv_shader_library(
    TARGET ARUI_OpenGLShaders
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Render/res/Shaders/OpenGL"
    TARGET_ENV opengl
    NAMESPACE ARUI::Shaders::OpenGL
)
add_subdirectory(Core)
add_subdirectory(Protocol)
add_subdirectory(Language)
add_subdirectory(Application)
add_subdirectory(Runtime)
add_subdirectory(Presentation)
add_subdirectory(Scripting)
add_subdirectory(Render)
add_subdirectory(XR)
add_subdirectory(Render/Backends/OpenGL/OpenXR)
add_subdirectory(Tools)
add_subdirectory(Server)
