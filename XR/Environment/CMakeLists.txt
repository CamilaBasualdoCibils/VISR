add_library(ARUIXREnvironment INTERFACE)
add_library(ARUI::XREnvironment ALIAS ARUIXREnvironment)
target_include_directories(ARUIXREnvironment INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIXREnvironment INTERFACE ARUI::Core)
