add_library(ARUIApplication INTERFACE)
add_library(ARUI::Application ALIAS ARUIApplication)

target_include_directories(ARUIApplication INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIApplication INTERFACE ARUI::Core ARUI::Protocol)
target_compile_features(ARUIApplication INTERFACE cxx_std_23)
