add_library(ARUIRuntime STATIC
    src/Painter.cpp
    src/RuntimePainter.cpp
    src/RuntimeTree.cpp)
add_library(ARUI::Runtime ALIAS ARUIRuntime)
target_include_directories(ARUIRuntime PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIRuntime PUBLIC
    ARUI::Core
    ARUI::Protocol
    ARUI::Language
    ARUI::Application)
target_compile_features(ARUIRuntime PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
