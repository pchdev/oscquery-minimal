add_executable(mempool ${WQUERY_TESTS_DIR}/mempool.c)
target_link_libraries(mempool ${PROJECT_NAME})
target_include_directories(mempool PRIVATE ${WQUERY_INCLUDE_DIR})
add_test(NAME mempool_unittest COMMAND mempool)

add_executable(osc ${WQUERY_TESTS_DIR}/osc.c)
target_link_libraries(osc ${PROJECT_NAME})
target_include_directories(osc PRIVATE ${WQUERY_INCLUDE_DIR})
add_test(NAME osc_unittest COMMAND osc)

add_executable(query ${WQUERY_TESTS_DIR}/oscquery.c)
target_link_libraries(query ${PROJECT_NAME})
target_include_directories(query PRIVATE ${WQUERY_INCLUDE_DIR})
add_test(NAME query_unittest COMMAND query)
