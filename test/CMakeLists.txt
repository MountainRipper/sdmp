add_executable(tree_recursive_test tree_recursive_test.cpp)
target_include_directories(tree_recursive_test PRIVATE ../sdmp/core)

add_executable(tinycom_test tinycom_test.cpp)
target_include_directories(tinycom_test PRIVATE ../sdmp/interface)

add_executable(glfw_player glfw_player.cpp)
target_compile_definitions(glfw_player PRIVATE CMAKE_FILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(glfw_player sdmp-easy sdmp glad mp-common glfw drm ssl )
