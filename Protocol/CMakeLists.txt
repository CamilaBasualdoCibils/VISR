add_library(VISRProtocol INTERFACE)
add_library(VISR::Protocol ALIAS VISRProtocol)
target_include_directories(VISRProtocol INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRProtocol INTERFACE VISR::Core)
