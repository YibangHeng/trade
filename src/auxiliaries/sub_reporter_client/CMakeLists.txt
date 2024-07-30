project(sub_reporter_client)

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${Boost_LIBRARIES}
        fmt::fmt
        muduo_base
        muduo_net
        z
        PRIVATE
        trade_types
)
