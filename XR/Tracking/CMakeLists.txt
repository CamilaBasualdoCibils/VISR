add_library(VISRXRTracking INTERFACE)
add_library(VISR::XRTracking ALIAS VISRXRTracking)
target_include_directories(VISRXRTracking INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRXRTracking INTERFACE VISR::Core)
