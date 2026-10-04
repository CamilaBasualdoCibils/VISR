add_library(VISRCore INTERFACE)
add_library(VISR::Core ALIAS VISRCore)
target_include_directories(VISRCore INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRCore INTERFACE Boost::describe glm::glm spdlog::spdlog)
