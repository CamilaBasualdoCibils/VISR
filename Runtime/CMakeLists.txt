add_library(ARUIRuntime INTERFACE)
add_library(ARUI::Runtime ALIAS ARUIRuntime)
target_include_directories(ARUIRuntime INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIRuntime INTERFACE
    ARUI::Core
    ARUI::Protocol
    ARUI::Language
    ARUI::Application)
