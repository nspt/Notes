cmake_minimum_required(VERSION 3.23)
project(learn_opengl_practice)

add_library(glad STATIC)
target_sources(glad
    PRIVATE
        third_party/glad/src/glad.c
    PUBLIC
        FILE_SET HEADERS
	BASE_DIRS third_party/glad/include/
	FILES
            third_party/glad/include/glad/glad.h
)

add_subdirectory(third_party/glfw-3.4)
add_subdirectory(00_基础环境搭建)
add_subdirectory(01_基本框架)
add_subdirectory(02_图形管线基础)
add_subdirectory(03_Shader输入与数据流)