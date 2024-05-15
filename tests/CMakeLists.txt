PROJECT(tests)

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        trade_types
        holder
        PUBLIC
        Catch2::Catch2
        PRIVATE
        date::date
        date::date-tz
        fmt::fmt
        libzmq-static
)

# Allow user to run tests with `make test` or `ctest`.
catch_discover_tests(${PROJECT_NAME})
