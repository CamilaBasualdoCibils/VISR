find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
add_executable(VISRPresentationTests PresentationTests.cpp)
target_link_libraries(VISRPresentationTests PRIVATE
    VISR::PresentationServer
    GTest::gtest_main)
gtest_discover_tests(VISRPresentationTests)
