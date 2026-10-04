add_library(VISRApplication INTERFACE)
add_library(VISR::Application ALIAS VISRApplication)

target_include_directories(VISRApplication INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRApplication INTERFACE
    VISR::Core
    VISR::Protocol
    VISR::Language)
target_compile_features(VISRApplication INTERFACE cxx_std_23)
