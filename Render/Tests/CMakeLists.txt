add_executable(ARUIRendererTests RendererTests.cpp)
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
target_link_libraries(ARUIRendererTests PRIVATE
    ARUI::Render
    GTest::gtest
    GTest::gtest_main)
gtest_discover_tests(ARUIRendererTests)
