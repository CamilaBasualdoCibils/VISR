add_library(VISRSimulator STATIC
    src/SimulatorApplication.cpp
    src/SimulatorPresenter.cpp
    src/SimulatorViewProvider.cpp
    src/SimulatorXRTracker.cpp)
add_library(VISR::Simulator ALIAS VISRSimulator)
target_include_directories(VISRSimulator PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRSimulator PUBLIC
    VISR::Runtime
    VISR::XRDisplay
    VISR::XRTracking
    VISR::OpenXR
    VISR::RenderOpenGLOpenXR
    VISR::RenderOpenGL
    imgui::imgui
    implot::implot
    glfw
    spdlog::spdlog)

add_executable(VisrSimulator src/main.cpp)
target_link_libraries(VisrSimulator PRIVATE
    VISR::Simulator
    VISR::RenderOpenGL)
visr_enable_runtime_instrumentation(VisrSimulator)
