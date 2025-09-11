cmake_minimum_required(VERSION 3.16.0)

list(
  APPEND
  CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(
  socket-activation-example
  VERSION 0.0.1
  LANGUAGES CXX C)

find_package(spdlog REQUIRED)


add_subdirectory(sockact-a)

# set(CMAKE_CXX_STANDARD 23)

# add_executable(example_a
#   spidev_test.c
# )

# install(TARGETS example_a DESTINATION "${CMAKE_INSTALL_PREFIX}")
