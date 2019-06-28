cmake_minimum_required (VERSION 2.8.11)
project (cmnalib)

include(GNUInstallDirs)

add_subdirectory (src)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-long-long -pedantic")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-long-long -pedantic")
endif()
