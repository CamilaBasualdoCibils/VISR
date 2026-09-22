add_library(ARUIScripting INTERFACE)
add_library(ARUI::Scripting ALIAS ARUIScripting)

target_include_directories(ARUIScripting INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIScripting INTERFACE
    ARUI::Application
    ARUI::Language)
