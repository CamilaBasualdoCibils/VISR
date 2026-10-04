find_package(rpclib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(ARUIPresentation STATIC
    src/RpcPresentationController.cpp
    src/Wire.cpp)
add_library(ARUI::Presentation ALIAS ARUIPresentation)
target_include_directories(ARUIPresentation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIPresentation
    PUBLIC ARUI::Language
    PRIVATE rpclib::rpc nlohmann_json::nlohmann_json)
target_compile_features(ARUIPresentation PUBLIC cxx_std_23)

add_library(ARUIPresentationServer STATIC
    src/PresentationRpcServer.cpp
    src/RuntimePresentationController.cpp)
add_library(ARUI::PresentationServer ALIAS ARUIPresentationServer)
target_include_directories(ARUIPresentationServer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUIPresentationServer
    PUBLIC ARUI::Presentation ARUI::Runtime
    PRIVATE rpclib::rpc)
target_compile_features(ARUIPresentationServer PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
