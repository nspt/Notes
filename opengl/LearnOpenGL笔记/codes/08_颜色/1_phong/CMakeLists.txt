add_executable(phong_color)
target_compile_features(phong_color PRIVATE cxx_std_20)
target_sources(phong_color
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
)
target_link_libraries(phong_color
    PRIVATE
        glad glfw stb_image glm
)