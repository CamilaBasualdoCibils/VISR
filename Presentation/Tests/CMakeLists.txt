find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
add_executable(ARUIPresentationTests PresentationTests.cpp)
target_link_libraries(ARUIPresentationTests PRIVATE
    ARUI::PresentationServer
    GTest::gtest_main)
gtest_discover_tests(ARUIPresentationTests)
