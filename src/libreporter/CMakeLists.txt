project(reporter)

file(GLOB_RECURSE SOURCES "*.cpp")

add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        trade_types
        PRIVATE
        date::date
        date::date-tz
        muduo_base
        muduo_net
        unofficial::mysql-connector-cpp::connector
        resolv
        z
)
