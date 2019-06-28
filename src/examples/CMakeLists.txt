

#add_executable(at_test examples/at_test.c ${cmnalib_SRC})
#target_link_libraries(at_test ${libraries})
#add_test(at_sierra_wireless_mc7455 at_sierra_wireless_mc7455)

#add_definitions(-DHAVE_STDINT_H)   # this was for iperf
add_executable(traffic_test src/traffic_test.c)
#target_link_libraries(traffic_test ${libraries} Threads::Threads)
target_link_libraries(traffic_test cmnalib ${CMAKE_THREAD_LIBS_INIT})

add_executable(enum_sw src/enum_sw.c)
#target_link_libraries(traffic_test ${libraries} Threads::Threads)
target_link_libraries(enum_sw cmnalib ${CMAKE_THREAD_LIBS_INIT})

add_executable(interface_test src/interface_test.c)
#target_link_libraries(traffic_test ${libraries} Threads::Threads)
target_link_libraries(interface_test cmnalib)

add_executable(param_log src/param_log.c)
#target_link_libraries(traffic_test ${libraries} Threads::Threads)
target_link_libraries(param_log cmnalib)
