cmake_minimum_required(VERSION 3.23)
project(learn_opengl_practice)

add_library(glad OBJECT)
target_sources(glad
    PRIVATE
        third_party/glad/src/glad.c
    PUBLIC
        FILE_SET HEADERS
	    BASE_DIRS third_party/glad/include/
	    FILES
            third_party/glad/include/glad/glad.h
)

add_library(stb_image OBJECT)
target_sources(stb_image
    PRIVATE
        third_party/stb_image/stb_image.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS third_party/stb_image/
        FILES
            third_party/stb_image/stb_image.h
)

add_library(glm INTERFACE)
target_include_directories(glm
    INTERFACE
        third_party/glm/
)

add_subdirectory(third_party/glfw-3.4)
add_subdirectory(00_基础环境搭建)
add_subdirectory(01_基本框架)
add_subdirectory(02_图形管线基础)
add_subdirectory(03_Shader输入与数据流)
add_subdirectory(04_Texture)
add_subdirectory(05_Transformation)