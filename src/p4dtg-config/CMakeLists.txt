cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
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
elseif(UNIX)
    add_compile_options(-fPIC)
endif()

project(p4dtg-config VERSION ${BUILD_VER} DESCRIPTION "p4dtg configuration tool" LANGUAGES CXX ${RC})

set(WIN_SRC_FILES "p4dtg-config.rc")

set(SRC_FILES
AttrEdit.cc
DTSEdit.cc
DTSTab.cc
FixMap.cc
Help.cc
ListSource.cc
MainDialog.cc
MapEdit.cc
MapTab.cc
NewMap.cc
P4DTG.cc
P4Edit.cc
P4Tab.cc
PickList.cc
SegEdit.cc
SelectMap.cc)

add_library(p4dtg-config-lib STATIC ${SRC_FILES})

if (WIN32)
    set(ID_OS "\"NTX64\"")
    set(SDK_SHARE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/Release/dtgshare.lib")
    set(SRC_SHARE "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgsrcshare.lib")
    set(EXTENSION "lib")
    set(EXTRA_LINK_FLAGS "crypt32.lib")
else ()
    set_target_properties(p4dtg-config-lib PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
    set(ID_OS "\"LINUX26X86_64\"")
    set(SDK_SHARE "${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/share/dtgshare.a")
    set(SRC_SHARE "${CMAKE_CURRENT_SOURCE_DIR}/../share/dtgsrcshare.a")
    set(EXTENSION "a")
    set(EXTRA_LINK_FLAGS "-lpthread -ldl")
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

############# Windows specific version manipulation #########

set(ID_REL "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}")
set(ID_PATCH "${CMAKE_PROJECT_VERSION_PATCH}")

if (WIN32)
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
#############################################################

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
                        $ENV{p4api_dir}/include/p4
                        $ENV{openssl_dir}/include
                        )

find_package(Qt5 COMPONENTS Core Gui Widgets)

target_link_libraries(p4dtg-config-lib PUBLIC
    ${SDK_SHARE}
    ${SRC_SHARE}
    $ENV{p4api_dir}/lib/libp4api.${EXTENSION}
    $ENV{openssl_dir}/libcrypto.${EXTENSION}
    $ENV{openssl_dir}/libssl.${EXTENSION}
    ${EXTRA_LINK_FLAGS})

add_executable(p4dtg-config dtgconfig.cc ${WIN_SRC_FILES})
if (WIN32)
    set_target_properties(p4dtg-config PROPERTIES WIN32_EXECUTABLE TRUE)
    set_target_properties(p4dtg-config PROPERTIES LINK_FLAGS "/FORCE:MULTIPLE")
elseif (UNIX)
    set_target_properties(p4dtg-config PROPERTIES LINK_FLAGS "-Wl,--allow-multiple-definition")
endif()
# Identical definitions of cp_string(char const*) in src/share/genutils.cc and sdk/share/dtg-str.cc

target_link_libraries(p4dtg-config PUBLIC p4dtg-config-lib Qt5::Widgets Qt5::Core Qt5::Gui)
