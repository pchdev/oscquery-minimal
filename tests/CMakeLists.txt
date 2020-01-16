
add_executable(mempool ${CMAKE_CURRENT_SOURCE_DIR}/mempool.c)
target_link_libraries(mempool ${PROJECT_NAME})
target_include_directories(mempool PRIVATE ${WPN_INCLUDE_DIR})
add_test(NAME mempool_unittest COMMAND mempool)

add_executable(osc ${CMAKE_CURRENT_SOURCE_DIR}/osc.c)
target_link_libraries(osc ${PROJECT_NAME})
target_include_directories(osc PRIVATE ${WPN_INCLUDE_DIR})

#add_executable(oscquery_unittest ${CMAKE_CURRENT_SOURCE_DIR}/oscquery.c)
#target_link_libraries(oscquery_unittest ${PROJECT_NAME})
#target_include_directories(oscquery_unittest PRIVATE ${WPN_INCLUDE_DIR})
