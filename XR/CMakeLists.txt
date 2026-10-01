add_subdirectory(Display)
add_subdirectory(Tracking)
add_subdirectory(Environment)
add_subdirectory(OpenXR)

add_library(ARUIXR INTERFACE)
add_library(ARUI::XR ALIAS ARUIXR)
target_link_libraries(ARUIXR INTERFACE
    ARUI::XRDisplay
    ARUI::XRTracking
    ARUI::XREnvironment
    ARUI::OpenXR)
