 
# Define library. Only source files here!
project(cmnalib)

file(GLOB cmnalib_SRC
    "src/at/*.c"
    "src/devices/*.c"
    "src/meas/*.c"
    "src/traffic/*.c"
    "src/util/*.c"
)

# build shared library only (legacy)
#add_library(cmnalib SHARED ${cmnalib_SRC})

# create object library
add_library(cmnalib_obj OBJECT ${cmnalib_SRC})
set_property(TARGET cmnalib_obj PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(cmnalib_static STATIC $<TARGET_OBJECTS:cmnalib_obj>)
add_library(cmnalib SHARED $<TARGET_OBJECTS:cmnalib_obj>)
# note: for Cmake >= 3.12 we can link against the cmnalib_obj instead of generators above

# shared and static libs shall have same output name (libcmanlib.so and libcmnalib.a)
set_target_properties(cmnalib_static PROPERTIES OUTPUT_NAME cmnalib CLEAN_DIRECT_OUTPUT 1)
set_target_properties(cmnalib PROPERTIES OUTPUT_NAME cmnalib CLEAN_DIRECT_OUTPUT 1)

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(cmnalib_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)
target_include_directories(cmnalib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)
# for Cmake >= 3.12 we can remove the code above and only use the code below
target_include_directories(cmnalib_obj PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

# If we have compiler requirements for this library, list them
# here
#target_compile_features(lib
#    PUBLIC cxx_auto_type
#    PRIVATE cxx_variadic_templates)

# Depend on libraries that are defined in the top-level file
target_link_libraries(cmnalib_static ${COMMON_LIBRARIES})
target_link_libraries(cmnalib ${COMMON_LIBRARIES})

# for Cmake >= 3.12 we can use the code below
#target_link_libraries(cmnalib_obj
#    ${COMMON_LIBRARIES}
#)
#target_link_libraries(cmnalib PRIVATE cmnalib_obj)
#target_link_libraries(cmnalib_static PRIVATE cmnalib_obj)

# 'make install' to the correct locations (provided by GNUInstallDirs).
# note: for Cmake >= 3.12 add cmnalib_obj as a 'phantom ' to maintain its deps in the EXPORT; no actual objects are exported.
install(TARGETS cmnalib cmnalib_static #cmnalib_obj
    EXPORT MyLibraryConfig
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

# testlib needs to be compiled with -DAT_MOCK option.
# currently, this requires re-compilation of all sources
add_executable(testlib
    test/devices/sierra_wireless_em7565/test_at.c
#    $<TARGET_OBJECTS:cmnalib_obj>
    ${cmnalib_SRC}
)
    
target_compile_definitions(testlib PUBLIC -DTEST_PATH=${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(testlib PUBLIC -DAT_MOCK)
    
target_include_directories(testlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)
target_link_libraries(testlib
    ${COMMON_LIBRARIES}
)

add_test(testlib testlib)
