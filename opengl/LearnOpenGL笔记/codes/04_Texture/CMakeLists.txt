add_executable(texture)
target_compile_features(texture PRIVATE cxx_std_20)
target_sources(texture
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
)
target_link_libraries(texture
    PRIVATE
        glad glfw stb_image glm
)