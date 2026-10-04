add_executable(VISRRendererTests RendererTests.cpp)
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
target_link_libraries(VISRRendererTests PRIVATE
    VISR::Render
    GTest::gtest
    GTest::gtest_main)
gtest_discover_tests(VISRRendererTests)
