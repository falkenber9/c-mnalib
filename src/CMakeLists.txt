#
# Copyright 2018 Robert Falkenberg
#

#cmake_minimum_required(VERSION 3.0)

set(CMAKE_VERBOSE_MAKEFILE on)
#set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
if(CMAKE_USE_PTHREADS_INIT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

#add_compile_options(-std=gnu99 -Wall -Werror -Wno-long-long -pedantic)

INCLUDE(FindPkgConfig)

pkg_check_modules(GLIB2		REQUIRED	glib-2.0)
pkg_check_modules(GIO2		REQUIRED	gio-2.0)
pkg_check_modules(LIBUDEV	REQUIRED	libudev)
#pkg_check_modules (CURL    )
#pkg_search_module (IPERF   iperf)
find_package(CURL	REQUIRED)
find_package(Threads	REQUIRED)
include_directories(${GLIB2_INCLUDE_DIRS} ${GIO2_INCLUDE_DIRS} ${LIBUDEV_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${IPERF_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS} ${GIO2_LIBRARY_DIRS} ${LIBUDEV_LIBRARY_DIRS} ${CURL_LIBRARIES} ${IPERF_LIBRARY_DIRS})

# let the compiler find included files from subdirectories
include_directories(".")

# collect all commonly used lib-dirs
set(COMMON_LIBRARIES ${GLIB2_LIBRARIES} ${LIBUDEV_LIBRARIES} ${CURL_LIBRARIES} m)
#message(${COMMON_LIBRARIES})

add_subdirectory(cmnalib)
add_subdirectory(examples)

#add_executable(qmi_test qmi/qmi_test.c)
#target_link_libraries(qmi_test qmi-glib ${GLIB2_LIBRARIES} ${GIO2_LIBRARIES})
##add_test(qmi_test qmi_test)


#file(GLOB cmnalib_SRC
#    "at/*.c"
#    "devices/*.c"
#    "meas/*.c"
#    "traffic/*.c"
#    "util/*.c"
#)


