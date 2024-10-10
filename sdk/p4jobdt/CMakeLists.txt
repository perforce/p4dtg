cmake_minimum_required(VERSION 3.16)

message( NOTICE " Platform: ${CMAKE_SYSTEM_NAME}" )
message( NOTICE " Cmake Src: ${CMAKE_SOURCE_DIR} Cmake Bin: ${CMAKE_BINARY_DIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT DEFINED BUILD_VER)
    set(BUILD_VER "2000.0.9999999")
    message( NOTICE "-- Build version not supplied using 2000.0.99999999, to override pass in -DBUILD_VER=<version>")
endif()

# This has to be called before project
if (WIN32)
    set(ID_OS "NTX64")
    set(CMAKE_GENERATOR_PLATFORM "x64")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    set(RC "RC")
else ()
    set(ID_OS "LINUX26X86_64")
    add_compile_options(-fPIC)
endif()


project(p4dtg-p4jobs VERSION ${BUILD_VER} DESCRIPTION "p4dtg p4 jobs plugin" LANGUAGES CXX C ${RC})
set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(WIN_SRC_FILES "p4jobs.rc")

set(SRC_FILES
DTG-mod-p4.cc
MyDTG.cc
MyDTGProj.cc
MyDTGDefect.cc
MyDTS.cc
P4MetaClient.cc
p4utils.cc
p4charcvt.cc
../share/dtg-utils.c
${WIN_SRC_FILES}
)


add_library(p4jobdt SHARED ${SRC_FILES} )

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

# P4Jobs specific version:
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
endif()

#############################################################

if(NOT DEFINED ENV{p4api_dir})
    message(FATAL_ERROR "Please provide a path to p4api directory in p4api_dir environment variable")
endif()

if (WIN32)
    #set_target_properties(p4jobdt PROPERTIES LINK_FLAGS "/WHOLEARCHIVE")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/Release/dtgshare.lib")
    set(LIB_CRYPTO "$ENV{openssl_dir}/libcrypto.lib")
    set(LIB_SSL "$ENV{openssl_dir}/libssl.lib")
    set(LIB_P4API "$ENV{p4api_dir}/lib/libp4api.lib")
    set(CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
    set(EXTRA_LINK_FLAGS "crypt32.lib" "Ws2_32.lib")

else ()
    set_target_properties(p4jobdt PROPERTIES LINK_FLAGS "-Wl,--whole-archive -Wl,--allow-multiple-definition -static-libgcc")
    set(SHARE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../share/dtgshare.a")
    set(LIB_CRYPTO "$ENV{openssl_dir}/libcrypto.a")
    set(LIB_SSL "$ENV{openssl_dir}/libssl.a")
    set(LIB_P4API "$ENV{p4api_dir}/lib/libp4api.a")
    set(EXTRA_LINK_FLAGS "-Wl,-no-whole-archive")
    set_source_files_properties(MyDTG.cc PROPERTIES COMPILE_OPTIONS "-Wno-literal-suffix")
endif ()


set(SHARE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../share/")
# Path to SDK include
set(SDK_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
# Path to openssl include
set(OPENSSL_INCLUDE "$ENV{openssl_dir}/include)")

set(P4API_INCLUDE "$ENV{p4api_dir}/include")


include_directories(${SHARE_INCLUDE}
                    ${SDK_INCLUDE}
                    ${P4API_INCLUDE}
                    ${OPENSSL_INCLUDE})

target_link_libraries(p4jobdt PUBLIC
                    ${SHARE_LIB}
                    ${LIB_P4API}
                    ${LIB_CRYPTO}
                    ${LIB_SSL}
                    ${EXTRA_LINK_FLAGS}
                    )