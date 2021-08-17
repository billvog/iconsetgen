cmake_minimum_required(VERSION 3.14)
project(iconsetgen C)

set(lib_dir ${CMAKE_SOURCE_DIR}/lib)
set(src_dir ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(${lib_dir}/FFmpeg)

file(GLOB src_files ${src_dir}/*.c)
file(GLOB header_files ${src_dir}/*.h)

add_executable(iconsetgen ${src_files} ${header_files})
target_link_libraries(iconsetgen FFmpeg)