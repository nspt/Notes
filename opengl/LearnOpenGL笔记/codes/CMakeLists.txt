cmake_minimum_required(VERSION 3.23)
project(learn_opengl_practice)

add_library(glad STATIC)
target_sources(glad
    PRIVATE
        glad/src/glad.c
    PUBLIC
        FILE_SET HEADERS
	BASE_DIRS glad/include/
	FILES
            glad/include/glad/glad.h
)

add_subdirectory(基础环境搭建)
