add_executable(3d_plane)
target_compile_features(3d_plane PRIVATE cxx_std_20)
target_sources(3d_plane
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
)
target_link_libraries(3d_plane
    PRIVATE
        glad glfw stb_image glm
)