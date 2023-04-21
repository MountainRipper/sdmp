cmake_minimum_required(VERSION 3.15)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_MACOSX_RPATH ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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

include_directories(../third_party/include)

SET(TARGET_ARCH  ${CMAKE_SYSTEM_PROCESSOR})

SET(SEARCHED_LIBS "")
SET(EXTERNAL_SOURCE "")

add_definitions(-D MR_LOG_DEAULT=MR_DEBUG)
add_definitions(-D SDMP_LOG_LEVEL=mp::Logger::LogLevel::kLogLevelDebug)

macro(search_library libName macroDefine debugSuffix)
    SET(libVar ${macroDefine}_LIB)
    SET(libNames "")
    LIST(APPEND libNames ${libName})

    if(${CMAKE_BUILD_TYPE} MATCHES "Debug" AND NOT "${debugSuffix}" STREQUAL "")
        LIST(PREPEND libNames ${libName}${debugSuffix})
    endif()

    find_library(${libVar} NAMES ${libNames} HINTS ${SEARCH_LIB_PATHS})
    if(${libVar} STREQUAL "${libVar}-NOTFOUND")
        message(STATUS "INFO: SEARCH_LIBRARY NOT Fount ${libNames}")
    else()
        message(STATUS "INFO: SEARCH_LIBRARY Fount ${libNames} : ${${libVar}}")
        add_definitions(-D ${macroDefine}=1)
        LIST(APPEND SEARCHED_LIBS ${${libVar}})
    endif()
endmacro(search_library)

message("search path is ${SEARCH_LIB_PATHS}")
#linux libraries
search_library(GLX HAVE_GLX "")
search_library(va HAS_VAAPI "")
search_library(va-glx HAS_VAAPI_GLX "")
#rockchip mpp for linux
search_library(rockchip_mpp HAS_ROCKCHIP_MPP "")
#ffmpeg
search_library("avutil" HAS_AVUTIL "")
search_library("avdevice" HAS_AVUTIL "")
search_library("avformat" HAS_AVFORMAT "")
search_library("avcodec" HAS_AVCODEC "")
search_library("swscale" HAS_SW_SCALE "")
search_library("swresample" HAS_SW_RESAMPLE "")
#libyuv
search_library(yuv HAS_LIBYUV "")
#luajit
search_library("luajit" HAS_LUAJIT "")
#spdlog
search_library(spdlog SPDLOG_COMPILED_LIB "d")


message(STATUS "INFO: LINK LIBRARIES:${SEARCHED_LIBS}")

file( GLOB SOURCE_INTERFACE interface/*.h)

file( GLOB SOURCE_DEPS core_deps/*.cpp core_deps/*.c core_deps/*.h)
file( GLOB SOURCE_SDL_MIXER core_deps/sdl_mixer/*.cpp core_deps/sdl_mixer/*.c core_deps/sdl_mixer/*.h)

file( GLOB SOURCE_CORE core/*.cpp core/*.c core/*.h)
file( GLOB SOURCE_CORE_FILTERS core_filters/*.cpp core_filters/*.c core_filters/*.h)
file( GLOB SOURCE_CORE_FILTERS_INTERNAL core_filters/internal/*.cpp core_filters/internal/*.c core_filters/internal/*.h)
file( GLOB SOURCE_CORE_STUBS core_stubs/*.cpp core_stubs/*.c core_stubs/*.h)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  #message(STATUS "dir='${dir}'")
endforeach()

add_library(sdmp STATIC
    ${SOURCE_INTERFACE}
    ${SOURCE_CORE}
    ${SOURCE_SYSTEM}
    ${SOURCE_CORE_FILTERS}
    ${SOURCE_CORE_FILTERS_INTERNAL}
    ${SOURCE_CORE_STUBS}
    ${SOURCE_DEPS}
    ${SOURCE_SDL_MIXER}
    ${EXTERNAL_SOURCE}
)
target_include_directories(sdmp PUBLIC ./interface ./interface/core_stubs)
target_include_directories(sdmp PRIVATE ./core ./core_deps ../textureio/include)

set_target_properties(
  sdmp
  PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(sdmp PRIVATE SDP_LIBRARY)

if(${MP_TARGET_OS} MATCHES "linux")
target_link_libraries(sdmp ${SEARCHED_LIBS})
elseif(${MP_TARGET_OS} MATCHES "windows")
target_link_libraries(sdmp ${SEARCHED_LIBS})
elseif(${MP_TARGET_OS} MATCHES "darwin" AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
target_link_libraries(sdmp ${SEARCHED_LIBS} c++ "-framework Cocoa" "-framework OpenGL" "-framework IOKit" "-framework CoreFoundation" "-framework CoreVideo")
endif()


add_library(
  projectWarning INTERFACE
)
target_compile_options(
  projectWarning INTERFACE
  $<$<CXX_COMPILER_ID:GNU>:$<BUILD_INTERFACE:-Wall;-Wextra;-Wpedantic>>
  $<$<CXX_COMPILER_ID:Clang>:$<BUILD_INTERFACE:-Weverything;-Wpedantic>>
  $<$<CXX_COMPILER_ID:MSVC>:/W0>
)
# install libsdmp.so
install(
    TARGETS sdmp
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET sdmp
                    POST_BUILD
                    COMMAND echo  "copy interface:${CMAKE_CURRENT_SOURCE_DIR}/interface to ${CMAKE_CURRENT_BINARY_DIR}/interface"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/interface  ${CMAKE_CURRENT_BINARY_DIR}/interface
                   )


