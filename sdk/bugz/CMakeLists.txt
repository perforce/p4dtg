cmake_minimum_required(VERSION 3.16)

message( NOTICE "-- Cmake Src: ${CMAKE_SOURCE_DIR} Cmake Bin: ${CMAKE_BINARY_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()


# This has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set(ID_OS "NTX64")
    set(CMAKE_BUILD_TYPE "Release")
else ()
    add_compile_options(-fPIC)
    set(ID_OS "LINUX26X86_64")
endif()

project(p4dtg-bugz VERSION ${BUILD_VER} DESCRIPTION "p4dt bugzilla plugin" LANGUAGES CXX C)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(SRC_FILES
MyDTG.cc
MyDTGProj.cc
MyDTGDefect.cc
MyDTS.cc
DTG-mod-bugz.cc
../share/dtg-utils.c
)

add_library(bugz3mysql5 SHARED ${SRC_FILES} )
target_compile_definitions(bugz3mysql5 PRIVATE $<$<CONFIG:Release>:bugz3mysql5_RELEASE=1>)

# Build Share library ahead of bugz, some hardcoded build commands here

if(WIN32)
    add_custom_command(TARGET bugz3mysql5 PRE_BUILD
        COMMAND cmake -G "${CMAKE_GENERATOR}" -A ${CMAKE_GENERATOR_PLATFORM} -DBUILD_VER=${BUILD_VER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../share/
        VERBATIM)
    add_custom_command(TARGET bugz3mysql5
        PRE_BUILD
        COMMAND cmake --build . --config Release
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../share/
        VERBATIM)
endif()

if (UNIX)
    set_target_properties(bugz3mysql5 PROPERTIES LINK_FLAGS "-Wl,--allow-multiple-definition -static-libgcc")
    set_source_files_properties(MyDTG.cc PROPERTIES COMPILE_OPTIONS "-Wno-literal-suffix")
endif ()

############# Perforce Version processing block #############

# Pass in the version of the project via -DCMAKE_PROJECT_VERSION=2023.1.9999999
set(ID_REL ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR})
set(ID_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
string(TIMESTAMP YEAR %Y)
string(TIMESTAMP MONTH %m)
string(TIMESTAMP DAY %d)

# Set up version strings
set(ID_VER "\"${ID_OS}/${ID_REL}/${ID_PATCH} ${YEAR} ${MONTH} ${DAY}\"")
message(NOTICE "-- Version string used: ${ID_VER}")
add_compile_definitions(ID_VER=${ID_VER})

#############################################################


if(NOT DEFINED ENV{mysql_dir})
    message(FATAL_ERROR "Please provide a path to libmysql directory in mysql_dir environment variable")
endif()

message("-- Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgshare.lib")
    set(MYSQL_LIB "$ENV{mysql_dir}/lib/libmysql.lib")
else ()
    set(MYSQL_LIB "$ENV{mysql_dir}/lib/libmysqlclient.a")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/dtgshare.a")
endif()

# Path to mysql include.
set(MYSQL_INCLUDE "$ENV{mysql_dir}/include")
set(SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../share/")
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

include_directories(
                    ${MYSQL_INCLUDE}
                    ${SHARE_INCLUDE}
                    ${SDK_INCLUDE})
target_link_libraries(bugz3mysql5 PUBLIC
                    ${MYSQL_LIB}
                    ${SHARE_LIB})