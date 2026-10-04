find_package(rpclib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(VISRPresentation STATIC
    src/RpcPresentationController.cpp
    src/Wire.cpp)
add_library(VISR::Presentation ALIAS VISRPresentation)
target_include_directories(VISRPresentation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRPresentation
    PUBLIC VISR::Language
    PRIVATE rpclib::rpc nlohmann_json::nlohmann_json)
target_compile_features(VISRPresentation PUBLIC cxx_std_23)

add_library(VISRPresentationServer STATIC
    src/PresentationRpcServer.cpp
    src/RuntimePresentationController.cpp)
add_library(VISR::PresentationServer ALIAS VISRPresentationServer)
target_include_directories(VISRPresentationServer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRPresentationServer
    PUBLIC VISR::Presentation VISR::Runtime
    PRIVATE rpclib::rpc)
target_compile_features(VISRPresentationServer PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
