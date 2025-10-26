# Function: enable_strict_warnings
# ----------------------------------------------------
# Description: Apply a set of strict compiler warnings to a specific target
#
# Usage: enable_strict_warnings(my_target)
#
# Parameters: target - The name of the CMake target (executable or library) to which the warning
# flags should be applied.
#

function(enable_strict_warnings target)

    target_compile_features(${target} PUBLIC cxx_std_23)

    # Strict warnings for C++
    target_compile_options(
        ${target}
        PUBLIC $<$<COMPILE_LANGUAGE:CXX>:
               -Wall
               -Wextra
               -Wpedantic
               -Wshadow
               -Wnon-virtual-dtor
               -Wold-style-cast
               -Wcast-align
               -Wunused
               -Woverloaded-virtual
               -Wconversion
               -Wsign-conversion
               -Wnull-dereference
               -Wduplicated-cond
               -Wduplicated-branches
               -Wformat=2
               -Wlogical-op
               -Wuseless-cast
               -Wdouble-promotion
               -Wfloat-equal
               -Winit-self
               -Wmissing-include-dirs
               -Wswitch-enum
               -Wstrict-null-sentinel
               -Wstrict-overflow=2
               -Wswitch-default
               -Wpointer-arith
               -Wno-unknown-pragmas
               -Werror
               -Werror=unused-result
               >)

    # Strict warnings for C
    target_compile_options(
        ${target}
        PUBLIC $<$<COMPILE_LANGUAGE:C>:
               -Wall
               -Wextra
               -Wpedantic
               -Wshadow
               -Wpointer-arith
               -Wcast-align
               -Wconversion
               -Wsign-conversion
               -Wmissing-prototypes
               -Wstrict-prototypes
               -Wold-style-definition
               -Wundef
               -Werror
               -Werror=unused-result
               >)

    target_compile_options(
        ${target}
        PUBLIC $<$<CONFIG:Debug>:-Og>
               $<$<CONFIG:Release>:-O2>
               -fstack-protector-strong
               -fstack-clash-protection
               -Wformat
               -Werror=format-security
               -Wdate-time
               -D_FORTIFY_SOURCE=2)

    target_link_options(
        ${target}
        PUBLIC
        -Wl,-z,relro
        -Wl,-z,now)
endfunction()
