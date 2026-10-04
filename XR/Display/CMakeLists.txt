add_library(VISRXRDisplay INTERFACE)
add_library(VISR::XRDisplay ALIAS VISRXRDisplay)
target_include_directories(VISRXRDisplay INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRXRDisplay INTERFACE VISR::Render glm::glm)
