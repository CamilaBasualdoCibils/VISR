add_library(VISRXREnvironment INTERFACE)
add_library(VISR::XREnvironment ALIAS VISRXREnvironment)
target_include_directories(VISRXREnvironment INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRXREnvironment INTERFACE VISR::Core)
