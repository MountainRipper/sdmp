add_executable(tree_recursive_test tree_recursive_test.cpp)
target_include_directories(tree_recursive_test PRIVATE ../sdmp/core)

add_executable(tinycom_test tinycom_test.cpp)
target_include_directories(tinycom_test PRIVATE ../sdmp/interface)

file( GLOB SOURCE_EXAMPLE example/*.cpp example/*.h)
add_executable(sdl_player ${SOURCE_EXAMPLE})
target_compile_definitions(sdl_player PRIVATE CMAKE_FILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" USE_GLES=0)
target_include_directories(sdl_player PRIVATE example/imgui)



if(NOT DEFINED ${MR_TARGET_OS})
    set(MR_TARGET_OS ${CMAKE_SYSTEM_NAME})
endif()
string(TOLOWER ${MR_TARGET_OS} MR_TARGET_OS)
if(NOT DEFINED  ${MR_TARGET_ARCH})
    set(MR_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()


if(${MR_TARGET_OS} MATCHES "linux")
    include_directories(/usr/include/drm)
    target_link_libraries(sdl_player sdmp-easy sdmp textureio glad mrcommon mrbundle SDL2 freetype drm ssl X11)
elseif(${MR_TARGET_OS} MATCHES "windows")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(MR_TARGET_OS "mingw")
        target_link_libraries(sdl_player sdmp-easy sdmp textureio glad mrcommon mrbundle SDL2.dll dwmapi luajit-5.1.dll )
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_link_libraries(sdl_player sdmp-easy sdmp textureio glad mrcommon mrbundle SDL2 dwmapi lua51 )
    endif()
elseif(${MR_TARGET_OS} MATCHES "darwin")

elseif(${MR_TARGET_OS} MATCHES "android")

elseif(${MR_TARGET_OS} MATCHES "ios")
endif()
