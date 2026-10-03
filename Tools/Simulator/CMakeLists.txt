add_library(ARUISimulator STATIC
    src/SimulatorApplication.cpp
    src/SimulatorPresenter.cpp
    src/SimulatorViewProvider.cpp
    src/SimulatorXRTracker.cpp)
add_library(ARUI::Simulator ALIAS ARUISimulator)
target_include_directories(ARUISimulator PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUISimulator PUBLIC
    ARUI::Runtime
    ARUI::XRDisplay
    ARUI::XRTracking
    ARUI::OpenXR
    ARUI::RenderOpenGLOpenXR
    ARUI::RenderOpenGL
    imgui::imgui
    implot::implot
    glfw
    spdlog::spdlog)

add_executable(AruiSimulator src/main.cpp)
target_link_libraries(AruiSimulator PRIVATE
    ARUI::Simulator
    ARUI::RenderOpenGL)
arui_enable_runtime_instrumentation(AruiSimulator)
