cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(SDMP)
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    set(CMAKE_CXX_FLAGS "-fPIC" )
    set(CMAKE_C_FLAGS "-fPIC" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    set(CMAKE_CXX_FLAGS "/FS /EHsc" )
    set(CMAKE_C_FLAGS "/FS /EHsc" )
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#######################arch and os detect#######################
if(NOT DEFINED ${MP_TARGET_OS})
    set(MP_TARGET_OS ${CMAKE_SYSTEM_NAME})
endif()
string(TOLOWER ${MP_TARGET_OS} MP_TARGET_OS)
if(NOT DEFINED  ${MP_TARGET_ARCH})
    set(MP_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()


if(${MP_TARGET_OS} MATCHES "linux")
    include_directories(/usr/include/drm)
elseif(${MP_TARGET_OS} MATCHES "windows")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(MP_TARGET_OS "mingw")
    endif()
elseif(${MP_TARGET_OS} MATCHES "darwin")

elseif(${MP_TARGET_OS} MATCHES "android")

elseif(${MP_TARGET_OS} MATCHES "ios")
endif()

if(${MP_TARGET_ARCH} MATCHES "AMD64")
    set(MP_TARGET_ARCH "x86_64")
endif()
#######################arch and os detect end#######################

#mountain rippers third_party library builds
set(THIRD_PARTY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party")
set(THIRD_PARTY_TARGET "${THIRD_PARTY_ROOT}/targets/${MP_TARGET_OS}-${MP_TARGET_ARCH}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    "${THIRD_PARTY_ROOT}/include"
    "${THIRD_PARTY_ROOT}/include/imgui"
    "${THIRD_PARTY_TARGET}/include"
    "${THIRD_PARTY_TARGET}/include/luajit"
    "${THIRD_PARTY_TARGET}/include/freetype2")
link_directories("${THIRD_PARTY_TARGET}/lib")
SET(SEARCH_LIB_PATHS "${THIRD_PARTY_TARGET}/lib")
message(STATUS "INFO:MAKE FOR OS:${MP_TARGET_OS} ARCH:${MP_TARGET_ARCH} SYSROOT:${CMAKE_SYSROOT}")
message(STATUS "INFO:MAKE USE PLATFORM THIRD-PARTY LIBS:${SEARCH_LIB_PATHS}")


if(NOT ${CMAKE_SYSROOT} STREQUAL "")
    foreach(PATH ${SEARCH_LIB_PATHS})
        get_filename_component(PARENT ${PATH} DIRECTORY)
        get_filename_component(LASTPART ${PATH} NAME)
        list(PREPEND CMAKE_FIND_ROOT_PATH ${PARENT})
        list(PREPEND CMAKE_SYSTEM_LIBRARY_PATH /${LASTPART})
        message(STATUS "INFO: ADD CROSS-COMPILE SERACH ROOT:${PARENT} DIR:/${LASTPART}")
    endforeach()
endif()
add_subdirectory(textureio)
add_subdirectory(sdmp)
add_subdirectory(easy)
add_subdirectory(test)
