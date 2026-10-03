
find_package(Boost REQUIRED CONFIG COMPONENTS
    container
    describe
    geometry
    graph
    multi_index
    stacktrace_addr2line
    static_string
    type_traits
    uuid
    program_options
    lockfree
)

find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
set(FG_BUILD_TEST OFF)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../extern/FrameGraph FrameGraph)