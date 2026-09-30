find_package(pugixml CONFIG REQUIRED)

add_library(ARUILanguage STATIC src/Parser.cpp src/Serializer.cpp src/Style.cpp)
add_library(ARUI::Language ALIAS ARUILanguage)
target_include_directories(ARUILanguage PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(ARUILanguage PUBLIC ARUI::Core PRIVATE pugixml::pugixml Boost::describe)
target_compile_features(ARUILanguage PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(Tests)
endif()
