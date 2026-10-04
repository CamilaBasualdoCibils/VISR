add_executable(VISRLanguageTests ParserTests.cpp NodeTests.cpp)
find_package(GTest CONFIG REQUIRED)

target_link_libraries(VISRLanguageTests PRIVATE
VISR::Language
GTest::gtest
GTest::gtest_main
GTest::gmock
GTest::gmock_main)
include(GoogleTest)
gtest_discover_tests(VISRLanguageTests)
