project(broker)

file(GLOB_RECURSE SOURCES "*.cpp")

add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        trade_types
        PRIVATE
        thostmduserapi_se
        thosttraderapi_se
        utapi
)
