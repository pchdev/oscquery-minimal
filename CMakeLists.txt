cmake_minimum_required(VERSION 3.5)
project(oscquery-minimal LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WPN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(WPN_HEADERS_DIR ${WPN_INCLUDE_DIR}/wpn114)
set(WPN_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(WPN_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(WPN_DEPENDENCIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

option(WPN_TESTS "enables unit-testing for this module" ON)

set(PROJECT_HEADER_FILES
    ${WPN_HEADERS_DIR}/wpn114.h
    ${WPN_HEADERS_DIR}/utilities.h
    ${WPN_HEADERS_DIR}/types.h
    ${WPN_HEADERS_DIR}/mempool.h)

set(PROJECT_SOURCE_FILES
    ${WPN_SOURCES_DIR}/mempool.c)

# OPTIONS -----------------------------------------------------------------------------------------

option(WPN_MULTITHREAD "enables/disables multithread processing" ON)

# HEADERS/SOURCES ---------------------------------------------------------------------------------

set(PROJECT_HEADER_FILES ${PROJECT_HEADER_FILES}
    ${WPN_HEADERS_DIR}/network/osc.h
    ${WPN_HEADERS_DIR}/network/oscquery.h
    ${WPN_DEPENDENCIES_DIR}/mongoose/mongoose.h
    ${WPN_DEPENDENCIES_DIR}/mjson/mjson.h)
set(PROJECT_SOURCE_FILES ${PROJECT_SOURCE_FILES}
    ${WPN_SOURCES_DIR}/network/osc.c
    ${WPN_SOURCES_DIR}/network/oscquery.c
    ${WPN_DEPENDENCIES_DIR}/mongoose/mongoose.c)

if (WPN_TESTS)
    add_subdirectory(tests)
endif()

# PROJECT -----------------------------------------------------------------------------------------

include(ProcessorCount)
ProcessorCount(WPN114_NPROC)

add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${WPN_INCLUDE_DIR})

if (WPN_MULTITHREAD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        -DWPN114_MULTITHREAD
        -DWPN114_NPROC=${WPN114_NPROC})
endif()

# LINK --------------------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} -lpthread)
