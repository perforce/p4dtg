cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.99999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

# This windows specific has to be called before project
if (WIN32)
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set(RC "RC")
endif ()

project(p4dtg-service VERSION ${BUILD_VER} DESCRIPTION "p4dtg service" LANGUAGES CXX ${RC})

set(SRC_FILES
Service.cc
Simple.cc)

add_library(p4dtg-service-lib STATIC ${SRC_FILES})

if (WIN32)
    set(ID_OS "\"NTX64\"")
    set(SDK_SHARE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/Release/dtgshare.lib")
    set(SRC_SHARE "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgsrcshare.lib")
    remove_definitions(-DQT_NO_CAST_FROM_ASCII)
endif ()

############# Windows specific version manipulation #########

set(ID_REL "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}")
set(ID_PATCH "${CMAKE_PROJECT_VERSION_PATCH}")

if (WIN32)
    string(TIMESTAMP YEAR "\"%Y\"")
    string(SUBSTRING ${ID_PATCH} 3 -1 LBUILD)
    string(SUBSTRING ${ID_PATCH} 0 3 HBUILD)
    add_compile_definitions(P4_INT_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR})
    add_compile_definitions(P4_INT_MINOR=${CMAKE_PROJECT_VERSION_MINOR})
    add_compile_definitions(P4_INT_HBUILD=${HBUILD})
    add_compile_definitions(P4_INT_LBUILD=${LBUILD})
    set(P4_FILE_VERSION "${ID_REL}.${HBUILD}.${LBUILD}")
    message("-- P4_FILE_VERSION used: ${P4_FILE_VERSION}")
    add_compile_definitions(P4_FILE_VERSION=${P4_FILE_VERSION})
    add_compile_definitions(P4_PRODUCT_VERSION=\"${P4_FILE_VERSION}\")
    add_compile_definitions(P4_COPYRIGHT=${YEAR})
endif ()

############# Perforce Version processing block #############

if(NOT DEFINED ENV{qt5_dir})
    message(FATAL_ERROR "Please provide a path to qt5 directory in qt5_dir environment variable")
endif()

set(CMAKE_PREFIX_PATH $ENV{qt5_dir})

set(SDK_SHARE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/")
set(SDK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include/")
set(SRC_SHARE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../share/")

include_directories(    $ENV{qt5_dir}/include/QtGui
                        $ENV{qt5_dir}/include/QtCore
                        $ENV{qt5_dir}/include/QtWidgets
                        $ENV{qt5_dir}/include
                        ${SRC_SHARE_DIR}
                        ${SDK_INCLUDE_DIR}
                        ${SDK_SHARE_DIR}
                        )

find_package(Qt5 COMPONENTS Core Gui Widgets)

target_link_libraries(p4dtg-service-lib PUBLIC
    ${SDK_SHARE}
    ${SRC_SHARE})

add_executable(p4dtg-service p4dtg-service.rc dtgservice.cc)
if (WIN32)
    set_target_properties(p4dtg-service PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(p4dtg-service PUBLIC p4dtg-service-lib Qt5::Widgets Qt5::Core Qt5::Gui)
