cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

project(p4dtg VERSION ${BUILD_VER} DESCRIPTION "p4dtg configuration tool" LANGUAGES C CXX)

if(NOT DEFINED ENV{p4api_dir})
    message(FATAL_ERROR "Please provide a path to p4api directory in p4api_dir environment variable")
endif()

if(NOT DEFINED ENV{openssl_dir})
    message(FATAL_ERROR "Please provide a path to openssl directory in openssl_dir environment variable")
endif()

if(NOT DEFINED ENV{qt5_dir})
    message(FATAL_ERROR "Please provide a path to qt5 directory in qt5_dir environment variable")
endif()

set(CMAKE_PREFIX_PATH $ENV{qt5_dir})
find_package(Qt5 COMPONENTS Core Gui Widgets)

# Build command used: cmake -DBUILD_VER="2023.2.1234567" -G "Visual Studio 15 2017" -A x64 build . ; cmake --build . --config Release

# Build Share library ahead of bugz, some hardcoded build commands here
if (WIN32)
    add_custom_target(share ALL)
    add_custom_command(TARGET share
        PRE_BUILD
        COMMAND cmake -G "${CMAKE_GENERATOR}" -A ${CMAKE_GENERATOR_PLATFORM} -DBUILD_VER=${BUILD_VER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/
        VERBATIM)
    add_custom_command(TARGET share
        PRE_BUILD
        COMMAND cmake --build . --config Release
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/
        VERBATIM)
    add_subdirectory("ntservice")
else ()
    add_subdirectory("share" "share")
endif ()

add_subdirectory("p4dtg-config")
add_subdirectory("p4dtg-repl")
