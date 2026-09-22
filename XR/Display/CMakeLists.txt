add_library(ARUIXRDisplay INTERFACE)
add_library(ARUI::XRDisplay ALIAS ARUIXRDisplay)
target_include_directories(ARUIXRDisplay INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIXRDisplay INTERFACE ARUI::Render glm::glm)
