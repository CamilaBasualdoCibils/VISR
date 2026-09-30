add_executable(ARUIRuntimeTests RuntimeTreeTests.cpp)
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)
target_link_libraries(ARUIRuntimeTests PRIVATE
    ARUI::Runtime
    GTest::gtest
    GTest::gtest_main)
gtest_discover_tests(ARUIRuntimeTests)
