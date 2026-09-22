add_library(ARUIXRTracking INTERFACE)
add_library(ARUI::XRTracking ALIAS ARUIXRTracking)
target_include_directories(ARUIXRTracking INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIXRTracking INTERFACE ARUI::Core)
