add_executable(ARUILanguageTests ParserTests.cpp NodeTests.cpp)
find_package(GTest CONFIG REQUIRED)

target_link_libraries(ARUILanguageTests PRIVATE 
ARUI::Language
GTest::gtest 
GTest::gtest_main 
GTest::gmock 
GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(ARUILanguageTests)
