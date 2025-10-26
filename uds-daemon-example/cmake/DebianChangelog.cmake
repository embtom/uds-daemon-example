# cmake-format: off
# Function: parse_debian_changelog(<changelog_file>)
#----------------------------------------------------
# Description:
#   Parses a Debian-style debian/changelog file
#     - DEBIAN_PROJECT_NAME     : package name
#     - DEBIAN_PROJECT_EPOCH    : epoch (defaults to 0 if not present)
#     - DEBIAN_PROJECT_VERSION  : upstream version (e.g. 1.2.3)
#     - DEBIAN_PROJECT_REVISION : Debian revision (e.g. -1, -2, etc.)
#
# Format of Debian version:
#     [epoch:]upstream_version[-debian_revision]
#
# Example:
#   some-prj (0.1.0-1) unstable; urgency=medium
#     * Initial release
#
#   Produces:
#     DEBIAN_PROJECT_NAME     = some-prj
#     DEBIAN_PROJECT_EPOCH    = 0
#     DEBIAN_PROJECT_VERSION  = 0.1.0
#     DEBIAN_PROJECT_REVISION = 1
#
# Reference:
#   https://www.debian.org/doc/debian-policy/ch-controlfields.html#version
# cmake-format: on

function(parse_debian_changelog CHANGELOG_FILE)
    file(
        READ
        "${CHANGELOG_FILE}"
        changelog)

    string(
        REGEX MATCH
              "^[a-z]+[a-z0-9+-.]+"
              project_name
              "${changelog}")

    string(
        REGEX MATCH
              "\\\((([0-9]+):)?(([0-9.]+)([A-Za-z0-9.+~]*))(-(([0-9.]*)([A-Za-z0-9.+~]*)))?\\\)"
              _
              ${changelog})

    set(epoch "${CMAKE_MATCH_2}")
    set(version "${CMAKE_MATCH_4}")
    set(revision "${CMAKE_MATCH_7}")

    # Epoch fallback to 0 if not found
    if(NOT epoch)
        set(epoch "0")
    endif()
    if(NOT revision)
        set(revision "0")
    endif()

    set(DEBIAN_PROJECT_EPOCH
        "${epoch}"
        PARENT_SCOPE)
    set(DEBIAN_PROJECT_VERSION
        "${version}"
        PARENT_SCOPE)
    set(DEBIAN_PROJECT_REVISION
        "${revision}"
        PARENT_SCOPE)
    set(DEBIAN_PROJECT_NAME
        "${project_name}"
        PARENT_SCOPE)

    message(STATUS "Set project name to: ${project_name}")
    message(STATUS "Set project epoch to: ${epoch}")
    message(STATUS "Set project version to: ${version}")
    message(STATUS "Set project revision to: ${revision}")
endfunction()
