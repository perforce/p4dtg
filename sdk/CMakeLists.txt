cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

project(p4dtg-sdk VERSION ${BUILD_VER} DESCRIPTION "p4dt configuration tool" LANGUAGES C CXX)

# Build Share library ahead of bugz, some hardcoded build commands here
if (WIN32)
    add_custom_target(share-temp ALL)
    add_custom_command(TARGET share-temp
        PRE_BUILD
        COMMAND cmake -G "${CMAKE_GENERATOR}" -A ${CMAKE_GENERATOR_PLATFORM} -DBUILD_VER=${BUILD_VER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/
        VERBATIM)
    add_custom_command(TARGET share-temp
        PRE_BUILD
        COMMAND cmake --build . --config Release
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/
        VERBATIM)
else ()
    add_subdirectory("share" "share")
endif ()

add_subdirectory("mysql" "mysql")
add_subdirectory("bugz" "bugz")
add_subdirectory("jira-rest" "jira-rest")
add_subdirectory("p4jobdt" "p4jobdt")
add_subdirectory("test" "test")
