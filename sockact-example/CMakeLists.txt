cmake_minimum_required(VERSION 3.16.0)

list(
    APPEND
    CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(DebianChangelog)
parse_debian_changelog("${CMAKE_SOURCE_DIR}/debian/changelog")

project(
    ${DEBIAN_PROJECT_NAME}
    VERSION ${DEBIAN_PROJECT_REVISION}
    LANGUAGES CXX C)

include(GNUInstallDirs)
include(EnableStrictWarnings)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(LibSystemd REQUIRED)
find_package(cxxopts REQUIRED)

option(ENABLE_TESTING "Build and enable tests" ON)

add_subdirectory(lib)
add_subdirectory(cli)
add_subdirectory(sockact-a)

if(ENABLE_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
