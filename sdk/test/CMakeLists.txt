cmake_minimum_required(VERSION 3.16)

message( NOTICE "-- Platform: ${CMAKE_SYSTEM_NAME}" )
message( NOTICE "-- Cmake Src: ${CMAKE_SOURCE_DIR} Cmake Bin: ${CMAKE_BINARY_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

# This windows specific has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set(ID_OS "\"NTX64\"")
else ()
    set(ID_OS "\"LINUX26X86_64\"")
endif ()

project(p4dtg-test VERSION ${BUILD_VER} DESCRIPTION "p4dtg test plugin" LANGUAGES CXX C)
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(SRC_FILES
../share/DTGModule.cc
../share/dtg-utils.c
dtgtest.cc
)

if (UNIX)
    set(EXTRA_LINK_FLAGS "-ldl")
endif ()

set(SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../share/")
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

include_directories(${SHARE_INCLUDE} ${SDK_INCLUDE})
add_executable(p4dtg-test ${SRC_FILES})

############# Perforce Version processing block #############

# Pass in the version of the project via -DCMAKE_PROJECT_VERSION=2023.1.9999999
set(ID_REL "\"${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}\"")
set(ID_PATCH "\"${CMAKE_PROJECT_VERSION_PATCH}\"")
string(TIMESTAMP YEAR "\"%Y\"")
string(TIMESTAMP MONTH "\"%m\"")
string(TIMESTAMP DAY "\"%d\"")

# Set up version strings
add_compile_definitions(ID_REL=${ID_REL})
add_compile_definitions(ID_PATCH=${ID_PATCH})
add_compile_definitions(ID_Y=${YEAR})
add_compile_definitions(ID_M=${MONTH})
add_compile_definitions(ID_D=${DAY})
add_compile_definitions(ID_OS=${ID_OS})

#############################################################

target_link_libraries(p4dtg-test PUBLIC ${EXTRA_LINK_FLAGS})