add_subdirectory(Display)
add_subdirectory(Tracking)
add_subdirectory(Environment)
add_subdirectory(OpenXR)

add_library(VISRXR INTERFACE)
add_library(VISR::XR ALIAS VISRXR)
target_link_libraries(VISRXR INTERFACE
    VISR::XRDisplay
    VISR::XRTracking
    VISR::XREnvironment
    VISR::OpenXR)
