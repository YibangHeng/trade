file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(tests ${SOURCES})

target_link_libraries(tests PUBLIC Catch2::Catch2)

# Allow user to run tests with `make test` or `ctest`.
catch_discover_tests(tests)
