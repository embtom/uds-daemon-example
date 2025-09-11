add_executable(sockact-a "sockact-a.cpp")

target_compile_features(sockact-a PRIVATE cxx_std_23)

target_link_libraries(sockact-a PRIVATE spdlog::spdlog)

install(TARGETS sockact-a RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
