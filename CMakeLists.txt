cmake_minimum_required(VERSION 3.9)

project(trade VERSION 0.0.1 LANGUAGES CXX)

# Initialize some default paths.
include(GNUInstallDirs)

# Define the minimum C++ standard that is required.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compilation options.
option(BUILD_PYTHON "Enable building of Python bindings" ON)
option(BUILD_DOCS "Enable building of documentation" ON)

set(CMAKE_MESSAGE_CONTEXT_SHOW ON)
set(CMAKE_MESSAGE_CONTEXT ${CMAKE_PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/include/info.h.ini
        ${CMAKE_SOURCE_DIR}/include/info.h
)

# Third-party libraries.
link_directories(lib)

find_package(
        Boost REQUIRED
        COMPONENTS
        program_options
)
find_package(date REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(unofficial-sqlite3 REQUIRED)
find_package(ZeroMQ REQUIRED)

# Include directories.
include_directories(include)

# Protocols.
add_subdirectory(trade_types)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/trade_types) # For generated *.pb.h.

# Muduo.
include_directories(include/third/muduo)

# Binaries and libraries.
add_subdirectory(src/libbooker)
add_subdirectory(src/libbroker)
add_subdirectory(src/libreporter)
add_subdirectory(src/libholder)
add_subdirectory(src/auxiliaries/mds)
add_subdirectory(src/auxiliaries/offline_booker)
add_subdirectory(src/auxiliaries/raw_md_recorder)
add_subdirectory(src/auxiliaries/seq_checker)
add_subdirectory(src/trade)

# Configuration files.
set(CONFIG_FILES
        ctp.ini
        cut.ini
        trade.ini
        README.txt
)

foreach (CONFIG_FILE ${CONFIG_FILES})
    configure_file(${CMAKE_SOURCE_DIR}/etc/${CONFIG_FILE}
            ${CMAKE_BINARY_DIR}/etc/${CONFIG_FILE}
            COPYONLY)
    install(FILES ${CMAKE_SOURCE_DIR}/etc/${CONFIG_FILE}
            DESTINATION /usr/local/etc
            COMPONENT config)
endforeach ()

# Tests.
include(CTest)
if (BUILD_TESTING)
    find_package(Catch2 REQUIRED)
    include(Catch)
    add_subdirectory(tests)
endif ()

# Documentation.
if (BUILD_DOCS)
    add_subdirectory(doc)
endif ()

# Python bindings.
if (BUILD_PYTHON)
    find_package(pybind11)

    pybind11_add_module(_trade python/trade/_trade.cpp)
    target_link_libraries(_trade PUBLIC)

    # Install the Python module shared library.
    install(TARGETS _trade DESTINATION .)
endif ()

# Add an alias target for use if this project is included as a subproject in another project.
# add_library(trade::trade ALIAS trade)

# Install targets and configuration.
install(
        TARGETS trade
        EXPORT trade-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
        EXPORT trade-config
        NAMESPACE trade::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/trade
)

install(
        DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# This prints a summary of found dependencies.
include(FeatureSummary)
feature_summary(WHAT ALL)
