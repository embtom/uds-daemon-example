
set(UTILS_NAME "utils")

option(ENABLE_LOCAL_UNWIND "Enable local unwinding of the call stack at signal handler" OFF)
if(ENABLE_LOCAL_UNWIND)
   message(STATUS "Local unwinding of the call stack at signal handler is enabled")
   find_package(absl REQUIRED)
   find_package(Libunwind REQUIRED)
endif()

set(HEADERS
   "include/errormsg.h"s
   "include/queue.h"
   "include/list.h"
   "include/signalhandler.h"
)

set(SOURCES
   "src/signalhandler.cpp"
)

add_library(${UTILS_NAME} STATIC
   ${HEADERS}
   ${SOURCES}
)

if(LOCAL_BUILD_RPATH_SET)
   set_target_properties(${UTILS_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

target_compile_features(${UTILS_NAME}
   PUBLIC cxx_std_23
)

set_target_properties(${UTILS_NAME} PROPERTIES
   POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${UTILS_NAME}
   PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if (ENABLE_LOCAL_UNWIND)
   target_compile_definitions(${UTILS_NAME}
      PRIVATE ENABLE_LOCAL_UNWIND
   )

   target_link_libraries(${UTILS_NAME}
      PRIVATE spdlog::spdlog
      PRIVATE libunwind::libunwind
      PRIVATE absl::demangle_internal
   )
else()
   target_link_libraries(${UTILS_NAME}
      PRIVATE spdlog::spdlog
   )
endif()
