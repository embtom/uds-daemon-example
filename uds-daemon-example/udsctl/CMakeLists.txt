add_executable(udsctl udsctl.cpp)

target_compile_features(udsctl PRIVATE cxx_std_23)

target_link_libraries(
    udsctl
    PRIVATE spdlog::spdlog
            cxxopts::cxxopts
            net)

install(TARGETS udsctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
