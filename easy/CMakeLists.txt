
file( GLOB SOURCE_EASY *.cpp *.c *.h)
add_library(sdmp-easy STATIC
${SOURCE_EASY}
synthesis.h synthesis.cpp
)
target_include_directories(sdmp-easy PUBLIC ./)
target_link_libraries(sdmp-easy sdmp)
