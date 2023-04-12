add_executable(tree_recursive_test tree_recursive_test.cpp)
target_include_directories(tree_recursive_test PRIVATE ../sdmp/core)

add_executable(tinycom_test tinycom_test.cpp)
target_include_directories(tinycom_test PRIVATE ../sdmp/interface)

file( GLOB SOURCE_PNG_PACKAGE png_merge.cpp)
add_executable(png_merge ${SOURCE_PNG_PACKAGE})


file( GLOB SOURCE_EXAMPLE example/*.cpp example/*.h)
file( GLOB SOURCE_IMGUI
    example/imgui/backends/imgui_impl_glfw.cpp
    example/imgui/backends/imgui_impl_opengl3.cpp
    example/imgui/*.cpp)
add_executable(glfw_example ${SOURCE_EXAMPLE} ${SOURCE_IMGUI})
target_compile_definitions(glfw_example PRIVATE CMAKE_FILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" USE_GLES=0)
target_include_directories(glfw_example PRIVATE example/imgui)
target_link_libraries(glfw_example sdmp-easy sdmp glad mr-common glfw freetype drm ssl X11)

target_include_directories(glfw_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../textureio/include)
