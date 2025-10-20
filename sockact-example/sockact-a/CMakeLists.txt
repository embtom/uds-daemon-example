add_executable(sockact-a "sockact-a.cpp")

target_compile_features(sockact-a PRIVATE cxx_std_23)

target_link_libraries(
    sockact-a
    PRIVATE libsystemd::libsystemd
    PRIVATE spdlog::spdlog
    PRIVATE utils
    PRIVATE net)

enable_strict_warnings(sockact-a)

install(TARGETS sockact-a RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
