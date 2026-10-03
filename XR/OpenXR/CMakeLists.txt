add_library(ARUIOpenXR STATIC
    src/OpenXRPresenter.cpp
    src/OpenXRTrackingProvider.cpp
    src/OpenXRViewProvider.cpp)
add_library(ARUI::OpenXR ALIAS ARUIOpenXR)
target_include_directories(ARUIOpenXR PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIOpenXR PUBLIC
    ARUI::XRDisplay
    ARUI::XRTracking
    ARUI::XREnvironment
    OpenXR::headers
    OpenXR::openxr_loader
    spdlog::spdlog)

