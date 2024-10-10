cmake_minimum_required(VERSION 3.16)


set(BUILD_SHARED_LIBS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/src-share/)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

# This windows specific has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif ()

project(p4dtg-share VERSION ${BUILD_VER} DESCRIPTION "p4dtg src shared library" LANGUAGES CXX)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

if(NOT DEFINED ENV{p4api_dir})
    message(FATAL_ERROR "Please provide a path to p4api directory in p4api_dir environment variable")
endif()

if (WIN32)
    set(EXTRA_SRC svcutils.cc)
endif ()

set(SRC_FILES
DataSource.cc
DataMapping.cc
DataAttr.cc
plugins.cc
scandir.cc
Settings.cc
DTGxml.cc
genutils.cc
Logger.cc
tinystr.cpp
tinyxml.cpp
tinyxmlerror.cpp
tinyxmlparser.cpp
${EXTRA_SRC})

add_library(dtgsrcshare STATIC ${SRC_FILES} )

if (WIN32)
    set(EXTENSION "lib")
else ()
    add_compile_options(-fPIC)
    set(EXTENSION "a")
endif ()

# path to lib dtgshare, required for all of the modules
set(SDK_SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/dtgshare.${EXTENSION}")
# path to share include
set(SDK_SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/")
# path to SDK include
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include")

include_directories(
                    ${SDK_SHARE_INCLUDE}
                    ${SDK_INCLUDE}
                    $ENV{p4api_dir}/include/p4)

target_link_libraries(dtgsrcshare PUBLIC ${SDK_SHARE_LIB})
