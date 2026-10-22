cmake_minimum_required(VERSION 3.14)
project(VISR VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VISR_ENABLE_TRACY "Enable Tracy profiling instead of mimalloc" OFF)
option(VISR_BUILD_TESTS "Build VISR tests" ON)
option(VISR_BUILD_TOOLS "Build VISR tools" ON)

if(NOT TARGET OpenPeripherals::Core)
    add_subdirectory(extern/OpenPeripherals)
endif()
include(cmake/Instrumentation.cmake)
include(cmake/FindDeps.cmake)
include(cmake/CompileShaders.cmake)
if(VISR_BUILD_TESTS)
    enable_testing()
endif()
add_spirv_shader_library(
    TARGET VISR_OpenGLShaders
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Render/res/Shaders/OpenGL"
    TARGET_ENV opengl
    NAMESPACE VISR::Shaders::OpenGL
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
if(VISR_BUILD_TOOLS)
    add_subdirectory(Tools)
    add_subdirectory(Server)
endif()
