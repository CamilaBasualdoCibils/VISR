add_library(ARUIServerManager STATIC src/Manager.cpp)
add_library(ARUI::ServerManager ALIAS ARUIServerManager)

target_include_directories(ARUIServerManager PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIServerManager
    PUBLIC
    ARUI::Runtime
    ARUI::Application
    ARUI::Scripting
    ARUI::Render
    ARUI::XR)
