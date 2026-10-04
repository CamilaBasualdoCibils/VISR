add_library(VISRRuntime STATIC
    src/IPainter.cpp
    src/RuntimePainter.cpp
    src/RuntimeTree.cpp
    src/VisrFlatPainter.cpp
    src/PainterCommonsLayout.cpp)
add_library(VISR::Runtime ALIAS VISRRuntime)
target_include_directories(VISRRuntime PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRRuntime PUBLIC
    VISR::Core
    VISR::Protocol
    VISR::Language
    VISR::Application)
target_compile_features(VISRRuntime PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
