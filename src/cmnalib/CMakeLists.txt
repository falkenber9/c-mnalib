 
# Define library. Only source files here!
project(cmnalib)

file(GLOB cmnalib_SRC
    "src/at/*.c"
    "src/devices/*.c"
    "src/meas/*.c"
    "src/traffic/*.c"
    "src/util/*.c"
)

add_library(cmnalib SHARED ${cmnalib_SRC})

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(cmnalib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

# If we have compiler requirements for this library, list them
# here
#target_compile_features(lib
#    PUBLIC cxx_auto_type
#    PRIVATE cxx_variadic_templates)

# Depend on a library that we defined in the top-level file
target_link_libraries(cmnalib
    ${GLIB2_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
    ${CURL_LIBRARIES}
    m)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS cmnalib EXPORT MyLibraryConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
install(EXPORT MyLibraryConfig DESTINATION share/cmnalib/cmake)

# This makes the project importable from the build directory
export(TARGETS cmnalib FILE MyLibraryConfig.cmake)

# Every library has unit tests, of course
#add_executable(testlib
#    test/testlib.cpp)

add_executable(testlib
    test/devices/sierra_wireless_em7565/test_at.c
    ${cmnalib_SRC})
    
target_compile_definitions(testlib PUBLIC -DTEST_PATH=${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(testlib PUBLIC -DAT_MOCK)
    
target_include_directories(testlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)
target_link_libraries(testlib
    ${GLIB2_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
    ${CURL_LIBRARIES}
    m)
#
#target_link_libraries(testlib
#    lib)
#
add_test(testlib testlib)
