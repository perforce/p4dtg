cmake_minimum_required(VERSION 3.16)

set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

message( NOTICE " Platform: ${CMAKE_SYSTEM_NAME}" )
message( NOTICE " Cmake Src: ${CMAKE_SOURCE_DIR} Cmake Bin: ${CMAKE_BINARY_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE " Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

# This has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set(ID_OS "\"NTX64\"")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgshare.lib")
else ()
    add_compile_options(-fPIC)
    set(ID_OS "\"LINUX26X86_64\"")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/dtgshare.a")
endif()

project(p4dtg-mydts VERSION ${BUILD_VER} DESCRIPTION "p4dt mydts plugin" LANGUAGES CXX)

set(SRC_FILES
MyDTG.cc
MyDTGProj.cc
MyDTGDefect.cc
MyDTS.cc
DTG-mod-mydts.cc
)

add_library(dtgmydts SHARED ${SRC_FILES})

# Path to share include
set(SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../share/")
# Path to SDK include
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

if (WIN32)
    set_target_properties(dtgmydts PROPERTIES LINK_FLAGS "/FORCE:MULTIPLE")
else ()
    set_target_properties(dtgmydts PROPERTIES LINK_FLAGS "-Wl,--whole-archive -Wl,--allow-multiple-definition -static-libgcc")
endif ()

include_directories(
                    ${SHARE_INCLUDE}
                    ${SDK_INCLUDE})
target_link_libraries(dtgmydts PUBLIC
                    ${SHARE_LIB})