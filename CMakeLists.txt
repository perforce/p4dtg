cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

project(p4dtg VERSION ${BUILD_VER} DESCRIPTION "p4dt configuration tool" LANGUAGES C CXX)


# Have problems with passing the windows build command not respecting order and building in alphabetical order
# Need to build share libraries manually ahead of this build
# Build command used: cmake -DBUILD_VER="2023.2.1234567" -G "Visual Studio 15 2017" -A x64 build . ; cmake --build . --config Release


add_subdirectory("sdk" "sdk")
add_subdirectory("src" "src")
