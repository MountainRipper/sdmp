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
target_link_libraries(sdl_player sdmp-easy sdmp textureio glad mr-common SDL2 freetype drm ssl X11)
target_include_directories(sdl_player PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../textureio/include)
