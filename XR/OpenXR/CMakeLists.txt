add_library(VISROpenXR STATIC
    src/OpenXRPresenter.cpp
    src/OpenXRTrackingProvider.cpp
    src/OpenXRViewProvider.cpp)
add_library(VISR::OpenXR ALIAS VISROpenXR)
target_include_directories(VISROpenXR PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISROpenXR PUBLIC
    VISR::XRDisplay
    VISR::XRTracking
    VISR::XREnvironment
    OpenXR::headers
    OpenXR::openxr_loader
    spdlog::spdlog)

