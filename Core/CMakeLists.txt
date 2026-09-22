add_library(ARUICore INTERFACE)
add_library(ARUI::Core ALIAS ARUICore)
target_include_directories(ARUICore INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUICore INTERFACE Boost::describe glm::glm)
