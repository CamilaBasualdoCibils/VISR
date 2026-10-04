add_executable(VISRRuntimeTests RuntimeTreeTests.cpp PainterTests.cpp)
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
target_link_libraries(VISRRuntimeTests PRIVATE
    VISR::Runtime
    GTest::gtest
    GTest::gtest_main)
gtest_discover_tests(VISRRuntimeTests)
