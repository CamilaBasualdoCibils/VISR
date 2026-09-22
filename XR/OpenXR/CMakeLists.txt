add_library(ARUIOpenXR INTERFACE)
add_library(ARUI::OpenXR ALIAS ARUIOpenXR)
target_include_directories(ARUIOpenXR INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIOpenXR INTERFACE
    ARUI::XRDisplay
    ARUI::XRTracking
    OpenXR::headers
    OpenXR::openxr_loader)
