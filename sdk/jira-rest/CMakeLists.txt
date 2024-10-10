cmake_minimum_required(VERSION 3.16)

set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to overrride pass in -DBUILD_VER=<version>")
endif()

# This has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgshare.lib")
    set(ID_OS "NTX64")
    # Just looking for defined or not here, so any value will do
    add_compile_definitions(OS_NT="true")
    set(EXTRA_LINK_FLAGS "wsock32")
else ()
    add_compile_options(-fPIC)
    set(ID_OS "LINUX26X86_64")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/dtgshare.a")
    # Silence the warnings about lack of spaces between literal and string macro.
    set_source_files_properties(MyDTG.cc PROPERTIES COMPILE_OPTIONS "-Wno-literal-suffix")
    set(EXTRA_LINK_FLAGS "-Wl,-no-whole-archive")
endif()

project(p4dtg-jirarest VERSION ${BUILD_VER} DESCRIPTION "p4dtg jirarest plugin" LANGUAGES CXX)

set(SRC_FILES
DTG-mod-jirarest.cc
tinystr.cpp
tinyxml.cpp
tinyxmlerror.cpp
tinyxmlparser.cpp
MyDTG.cc
MyDTGProj.cc
MyDTGDefect.cc
MyDTS.cc
TcpXML.cc
)

add_library(jirarest SHARED ${SRC_FILES} )

if (unix)
    # Silence the warnings about lack of spaces between literal and string macro.
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


# Path to share include
set(SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../share/")
# Path to SDK include
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

include_directories(
                    ${SHARE_INCLUDE}
                    ${SDK_INCLUDE})
target_link_libraries(jirarest PUBLIC
                    ${SHARE_LIB}
                    ${EXTRA_LINK_FLAGS})