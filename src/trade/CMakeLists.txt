project(trade)

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

find_package(fmt REQUIRED)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        fmt::fmt
)
