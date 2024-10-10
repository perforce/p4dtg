cmake_minimum_required(VERSION 3.16)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

message( NOTICE "Platform: ${CMAKE_SYSTEM_NAME}" )
message( NOTICE "Cmake Src: ${CMAKE_SOURCE_DIR} Cmake Bin: ${CMAKE_BINARY_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

if(NOT DEFINED ENV{p4api_dir})
    message(FATAL_ERROR "Please provide a path to p4api directory in p4api_dir environment variable")
endif()

if(NOT DEFINED ENV{openssl_dir})
    message(FATAL_ERROR "Please provide a path to openssl directory in openssl_dir environment variable")
endif()

# This windows specific has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif ()

project(p4dtg-repl VERSION ${BUILD_VER} DESCRIPTION "p4dtg replication engine" LANGUAGES CXX)

set(SRC_FILES
Unify.cc
process.cc
utils.cc
)

if (WIN32)
    set(ID_OS "\"NTX64\"")
    set(EXTENSION "lib")
    set(SDK_SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/Release/dtgshare.lib")
    set(SRC_SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgsrcshare.lib")
    set(EXTRA_LINK_FLAGS "crypt32.lib" "Ws2_32.lib")
else ()
    set(ID_OS "\"LINUX26X86_64\"")
    set(SDK_SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/dtgshare.a")
    set(SRC_SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/dtgsrcshare.a")
    set(EXTENSION "a")
    set(EXTRA_LINK_FLAGS "-lpthread -ldl")
    add_compile_options(-fPIC)
endif ()

############# Perforce Version processing block #############

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

set(SDK_SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/")
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include")
set(SRC_SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../share/")

add_library(dtgrepl STATIC ${SRC_FILES})
include_directories(
                    ${SDK_SHARE_INCLUDE}
                    ${SDK_INCLUDE}
                    ${SRC_SHARE_INCLUDE}
                    $ENV{p4api_dir}/include/p4
                    $ENV{openssl_dir}/include)

target_link_libraries(dtgrepl PUBLIC
                    ${SRC_SHARE_LIB}
                    ${SDK_SHARE_LIB}
                    $ENV{p4api_dir}/lib/libp4api.${EXTENSION}
                    $ENV{openssl_dir}/libcrypto.${EXTENSION}
                    $ENV{openssl_dir}/libssl.${EXTENSION}
                    ${EXTRA_LINK_FLAGS})

add_executable(p4dtg-repl dtgrepl.cc)
if (WIN32)
    set_target_properties(p4dtg-repl PROPERTIES WIN32_EXECUTABLE TRUE)
endif ()
target_link_libraries(p4dtg-repl PUBLIC dtgrepl)