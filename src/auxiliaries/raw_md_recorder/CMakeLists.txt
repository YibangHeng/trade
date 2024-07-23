project(raw_md_recorder)

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${Boost_LIBRARIES}
        fmt::fmt
        pcap
        PRIVATE
        broker
)
