add_library(VISRScripting INTERFACE)
add_library(VISR::Scripting ALIAS VISRScripting)

target_include_directories(VISRScripting INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRScripting INTERFACE
    VISR::Application
    VISR::Language)
