set(UTILS_NAME "utils")

set(HEADERS
    "include/errormsg.h"
    "include/fdset.h"
    "include/fs_utils.h"
    "include/pipe.h"
    "include/queue.h"
    "include/list.h"
    "include/signalhandler.h"
    "include/sd_notify.h"
    "include/string_utils.h")

set(SOURCES
    "src/signalhandler.cpp"
    "src/sd_notify.cpp"
    "src/fdset.cpp"
    "src/pipe.cpp")

add_library(${UTILS_NAME} STATIC ${HEADERS} ${SOURCES})

enable_strict_warnings(${UTILS_NAME})

set_target_properties(${UTILS_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    ${UTILS_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_link_libraries(${UTILS_NAME} PRIVATE spdlog::spdlog)

# if(LOCAL_BUILD_RPATH_SET) set_target_properties(${UTILS_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
# endif()
