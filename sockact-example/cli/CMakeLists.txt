add_executable(sockact-cli sockact-cli.cpp)

target_compile_features(sockact-cli PRIVATE cxx_std_23)

target_link_libraries(
    sockact-cli
    PRIVATE spdlog::spdlog
            cxxopts::cxxopts
            net)

install(TARGETS sockact-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
