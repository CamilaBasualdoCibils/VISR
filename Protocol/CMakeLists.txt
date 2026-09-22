add_library(ARUIProtocol INTERFACE)
add_library(ARUI::Protocol ALIAS ARUIProtocol)
target_include_directories(ARUIProtocol INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIProtocol INTERFACE ARUI::Core)
