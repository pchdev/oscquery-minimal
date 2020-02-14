cmake_minimum_required(VERSION 3.5)
project(wquery LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WQUERY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(WQUERY_HEADERS_DIR ${WQUERY_INCLUDE_DIR}/wpn114)
set(WQUERY_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(WQUERY_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(WQUERY_DEPENDENCIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# HEADERS/SOURCES ---------------------------------------------------------------------------------

set(PROJECT_HEADER_FILES
    ${WQUERY_HEADERS_DIR}/wpn114.h
    ${WQUERY_HEADERS_DIR}/utilities.h
    ${WQUERY_HEADERS_DIR}/types.h
    ${WQUERY_HEADERS_DIR}/alloc.h
    ${WQUERY_HEADERS_DIR}/network/osc.h
    ${WQUERY_HEADERS_DIR}/network/oscquery.h
    ${WQUERY_HEADERS_DIR}/network/zeroconf.h
    ${WQUERY_DEPENDENCIES_DIR}/mongoose/mongoose.h
    ${WQUERY_DEPENDENCIES_DIR}/mjson/mjson.h)

set(PROJECT_SOURCE_FILES
    ${WQUERY_SOURCES_DIR}/alloc.c
    ${WQUERY_SOURCES_DIR}/network/osc.c
    ${WQUERY_SOURCES_DIR}/network/oscquery.c
    ${WQUERY_SOURCES_DIR}/network/zeroconf.c
    ${WQUERY_DEPENDENCIES_DIR}/mongoose/mongoose.c)

# OPTIONS -----------------------------------------------------------------------------------------
include(ProcessorCount)
ProcessorCount(WQUERY_NPROC)
option(WQUERY_MULTITHREAD "enables/disables multithread processing" ON)
option(WQUERY_TESTS "enables unit-testing for this module" ON)
option(WQUERY_EXAMPLES "adds examples to compilation targets" ON)

set(WQUERY_MAX_CONNECTIONS 2)

if (WQUERY_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

if (WQUERY_EXAMPLES)
    add_subdirectory(examples)
endif()

# PROJECT -----------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${WQUERY_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    -DWQUERY_MAX_CONNECTIONS=${WQUERY_MAX_CONNECTIONS}
    -DWQUERY_MULTITHREAD
    -DWQUERY_NPROC=${WQUERY_NPROC})

# LINK --------------------------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} -lpthread)
