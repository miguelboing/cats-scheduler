# Test configuration
find_package(GTest QUIET)

if(GTest_FOUND)
    # Enable Google Test
    include(GoogleTest)
    
    # Test executable
    add_executable(scheduler_tests
        test_scheduler.cpp
        test_simulator.cpp
    )
    
    # Link libraries
    target_link_libraries(scheduler_tests 
        scheduler_lib
        GTest::gtest 
        GTest::gtest_main
    )
    
    # Discover tests
    gtest_discover_tests(scheduler_tests)
    
    message(STATUS "Google Test found - building tests")
else()
    message(STATUS "Google Test not found - skipping tests")
    
    # Create a simple test runner without GTest
    add_executable(simple_tests simple_test.cpp)
    target_link_libraries(simple_tests scheduler_lib)
    add_test(NAME simple_tests COMMAND simple_tests)
endif()