add_executable(tree_recursive_test tree_recursive_test.cpp)
target_include_directories(tree_recursive_test PRIVATE ../sdmp/core)

add_executable(tinycom_test tinycom_test.cpp)
target_include_directories(tinycom_test PRIVATE ../sdmp/interface)

file( GLOB SOURCE_EXAMPLE example/*.cpp example/*.h)
file( GLOB SOURCE_IMGUI
    example/imgui/backends/imgui_impl_glfw.cpp
    example/imgui/backends/imgui_impl_opengl3.cpp
    example/imgui/*.cpp)
add_executable(sdl_player ${SOURCE_EXAMPLE} ${SOURCE_IMGUI})
target_compile_definitions(sdl_player PRIVATE CMAKE_FILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" USE_GLES=0)
target_include_directories(sdl_player PRIVATE example/imgui)



if(NOT DEFINED ${MP_TARGET_OS})
    set(MP_TARGET_OS ${CMAKE_SYSTEM_NAME})
endif()
string(TOLOWER ${MP_TARGET_OS} MP_TARGET_OS)
if(NOT DEFINED  ${MP_TARGET_ARCH})
    set(MP_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()


if(${MP_TARGET_OS} MATCHES "linux")
    include_directories(/usr/include/drm)
    target_link_libraries(sdl_player sdmp-easy sdmp textureio glad mr-common SDL2 freetype drm ssl X11)
elseif(${MP_TARGET_OS} MATCHES "windows")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(MP_TARGET_OS "mingw")
        target_link_libraries(sdl_player sdmp-easy sdmp textureio glad mr-common SDL2.dll dwmapi luajit-5.1.dll )
    endif()
elseif(${MP_TARGET_OS} MATCHES "darwin")

elseif(${MP_TARGET_OS} MATCHES "android")

elseif(${MP_TARGET_OS} MATCHES "ios")
endif()
