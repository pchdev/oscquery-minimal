add_executable(mempool ${WPN_TESTS_DIR}/mempool.c)
target_link_libraries(mempool ${PROJECT_NAME})
target_include_directories(mempool PRIVATE ${WPN_INCLUDE_DIR})
add_test(NAME mempool_unittest COMMAND mempool)

add_executable(osc ${WPN_TESTS_DIR}/osc.c)
target_link_libraries(osc ${PROJECT_NAME})
target_include_directories(osc PRIVATE ${WPN_INCLUDE_DIR})
add_test(NAME osc_unittest COMMAND osc)

add_executable(oscquery ${WPN_TESTS_DIR}/oscquery.c)
target_link_libraries(oscquery ${PROJECT_NAME})
target_include_directories(oscquery PRIVATE ${WPN_INCLUDE_DIR})
