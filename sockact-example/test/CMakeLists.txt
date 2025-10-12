find_package(GTest REQUIRED)

set(TEST_SOURCES
    "test_main.cpp"
    "utils/test_fdset.cpp"
    "net/test_socket.cpp"
    "net/test_uds_server.cpp"
    "net/test_uds_client.cpp"
    "net/test_uds_server.h")

add_executable(unit_tests ${TEST_SOURCES})

target_link_libraries(
    unit_tests
    PRIVATE utils
            net
            Threads::Threads
            GTest::gtest_main)

enable_strict_warnings(unit_tests)

target_include_directories(unit_tests PRIVATE "net")

include(GoogleTest)
gtest_discover_tests(unit_tests)
