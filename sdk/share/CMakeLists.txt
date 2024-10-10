cmake_minimum_required(VERSION 3.16)

set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

message( NOTICE "-- Platform: ${CMAKE_SYSTEM_NAME}" )
message( NOTICE "-- Cmake Src: ${CMAKE_SOURCE_DIR} Cmake Bin: ${CMAKE_BINARY_DIR}")

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

# This has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
else ()
    add_compile_options(-fPIC)
endif()

project(p4dtg-share VERSION ${BUILD_VER} DESCRIPTION "p4dtg sdk shared library" LANGUAGES CXX C)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

set(SRC_FILES
DTGModule.cc
dtg-str.cc
dtg-utils.c
)

add_library(dtgshare STATIC ${SRC_FILES})
include_directories("../include")
