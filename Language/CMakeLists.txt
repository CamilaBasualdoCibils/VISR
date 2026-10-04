find_package(pugixml CONFIG REQUIRED)

add_library(VISRLanguage STATIC src/Parser.cpp src/Serializer.cpp src/Style.cpp)
add_library(VISR::Language ALIAS VISRLanguage)
target_include_directories(VISRLanguage PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(VISRLanguage PUBLIC VISR::Core PRIVATE pugixml::pugixml Boost::describe)
target_compile_features(VISRLanguage PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
