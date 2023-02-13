add_executable(tinycom_test tinycom_test.cpp)
target_include_directories(tinycom_test PRIVATE ../interface)


add_executable(glfw_player glfw_player.cpp)
target_compile_definitions(glfw_player PRIVATE CMAKE_FILE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(glfw_player sdp glfw drm ssl)
target_include_directories(glfw_player PRIVATE ../sdp/core_deps)