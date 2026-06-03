add_executable(3d_cubes)
target_compile_features(3d_cubes PRIVATE cxx_std_20)
target_sources(3d_cubes
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
)
target_link_libraries(3d_cubes
    PRIVATE
        glad glfw stb_image glm
)