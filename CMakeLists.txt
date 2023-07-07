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

#######################arch/os/3rd_party detect#######################
if(NOT DEFINED MR_TARGET_OS)
    set(MR_TARGET_OS ${CMAKE_SYSTEM_NAME})
endif()
string(TOLOWER ${MR_TARGET_OS} MR_TARGET_OS)
if(NOT DEFINED  MR_TARGET_ARCH)
    set(MR_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()


if(${MR_TARGET_OS} MATCHES "linux")
    include_directories(/usr/include/drm)
elseif(${MR_TARGET_OS} MATCHES "windows")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(MR_COMPILER_VER "mingw")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        if ("${MSVC_TOOLSET_VERSION}" STREQUAL "143")
            set(MR_COMPILER_VER "msvc2022")
        elseif ("${MSVC_TOOLSET_VERSION}" STREQUAL "142")
            set(MR_COMPILER_VER "msvc2019")
        elseif ("${MSVC_TOOLSET_VERSION}" STREQUAL "141")
            set(MR_COMPILER_VER "msvc2017")
        endif()
    endif()
elseif(${MR_TARGET_OS} MATCHES "darwin")

elseif(${MR_TARGET_OS} MATCHES "android")

elseif(${MR_TARGET_OS} MATCHES "ios")
endif()

if(${MR_TARGET_ARCH} MATCHES "AMD64")
    set(MR_TARGET_ARCH "x86_64")
endif()

if(NOT DEFINED MR_THIRD_PARTY_ROOT)
    set(MR_THIRD_PARTY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party")
    set(MR_THIRD_PARTY_TARGET "${MR_THIRD_PARTY_ROOT}/targets/${MR_BUILD_TYPE}/${MR_TARGET_OS}-${MR_COMPILER_VER}-${MR_TARGET_ARCH}")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        "${MR_THIRD_PARTY_ROOT}/include"
        "${MR_THIRD_PARTY_ROOT}/include/imgui"
        "${MR_THIRD_PARTY_TARGET}/include"
        "${MR_THIRD_PARTY_TARGET}/include/luajit"
        "${MR_THIRD_PARTY_TARGET}/include/freetype2")
    link_directories("${MR_THIRD_PARTY_TARGET}/lib")
    SET(MR_SEARCH_LIB_PATHS "${MR_THIRD_PARTY_TARGET}/lib")
endif()

message(STATUS "INFO:SDMP MAKE FOR OS:${MR_TARGET_OS} ARCH:${MR_TARGET_ARCH} SYSROOT:${CMAKE_SYSROOT}")
message(STATUS "INFO:SDMP MAKE USE PLATFORM THIRD-PARTY LIBS:${MR_SEARCH_LIB_PATHS}")
#######################arch/os/3rd_party detect end#######################

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
