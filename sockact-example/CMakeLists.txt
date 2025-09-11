cmake_minimum_required(VERSION 3.16.0)

list(
    APPEND
    CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(
    socket-activation-example
    VERSION 0.0.1
    LANGUAGES CXX C)

include(GNUInstallDirs)
find_package(spdlog REQUIRED)

add_subdirectory(sockact-a)
