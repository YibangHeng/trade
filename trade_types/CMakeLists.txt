project(trade_types)

file(GLOB PROTOCOLS *.proto)

find_package(Protobuf REQUIRED)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${PROTOCOLS})

include_directories(${Protobuf_INCLUDE_DIRS})

add_library(${PROJECT_NAME} ${PROTO_SRC})

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        ${Protobuf_LIBRARIES}
)
