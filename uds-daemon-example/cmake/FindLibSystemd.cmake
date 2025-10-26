message(STATUS "checking for libsystemd...")

find_package(PkgConfig REQUIRED)
if(PKG_CONFIG_FOUND)
   pkg_check_modules(PC_LIBSYSTEMD libsystemd)
endif()

find_path(LIBSYSTEMD_INCLUDE_DIR
   NAMES           "systemd/sd-journal.h"
   PATHS           ${PC_LIBSYSTEMD_INCLUDEDIR}
   PATH_SUFFIXES   "include"
   DOC             "Try to find the libsystemd header"
)

find_library(LIBSYSTEMD_LIBRARY
   NAMES           "systemd"
   PATHS           ${PC_LIBSYSTEMD_LIBDIR}
   PATH_SUFFIXES   "lib"
   DOC             "Try to find libsystemd"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibSystemd
   DEFAULT_MSG
   LIBSYSTEMD_LIBRARY
   LIBSYSTEMD_INCLUDE_DIR
)

if(LibSystemd_FOUND)
   message(STATUS "libsystemd library found: ${LIBSYSTEMD_LIBRARY}")
   message(STATUS "libsystemd headers found at: ${LIBSYSTEMD_INCLUDE_DIR}")

   add_library(libsystemd::libsystemd UNKNOWN IMPORTED)
   set_target_properties(libsystemd::libsystemd PROPERTIES
      IMPORTED_LOCATION "${LIBSYSTEMD_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBSYSTEMD_INCLUDE_DIR}")
endif()
