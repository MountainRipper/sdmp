add_executable(tree_recursive_test tree_recursive_test.cpp)
target_include_directories(tree_recursive_test PRIVATE ../sdmp/core)

add_executable(tinycom_test tinycom_test.cpp)
target_include_directories(tinycom_test PRIVATE ../sdmp/interface)

file( GLOB SOURCE_EXAMPLE example/*.cpp example/*.h)
add_executable(glfw_example ${SOURCE_EXAMPLE})
target_compile_definitions(glfw_example PRIVATE CMAKE_FILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(glfw_example sdmp-easy sdmp glad mp-common glfw drm ssl )
