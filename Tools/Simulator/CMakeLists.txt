add_library(ARUISimulator STATIC src/SimulatorPresenter.cpp)
add_library(ARUI::Simulator ALIAS ARUISimulator)
target_include_directories(ARUISimulator PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUISimulator PUBLIC
    ARUI::XRDisplay
    ARUI::XRTracking
    ARUI::RenderOpenGL
    imgui::imgui
    implot::implot
    glfw
    spdlog::spdlog)
