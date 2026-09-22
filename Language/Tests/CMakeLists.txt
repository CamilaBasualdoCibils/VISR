add_executable(ARUILanguageTests ParserTests.cpp)
find_package(GTest CONFIG REQUIRED)

target_link_libraries(ARUILanguageTests PRIVATE 
ARUI::Language
GTest::gtest 
GTest::gtest_main 
GTest::gmock 
GTest::gmock_main)
add_test(NAME ARUILanguageTests COMMAND ARUILanguageTests)
