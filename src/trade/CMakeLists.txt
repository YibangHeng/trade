project(trade)

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${Boost_LIBRARIES}
        fmt::fmt
        PRIVATE
        broker
)
