add_executable(transform)
target_compile_features(transform PRIVATE cxx_std_20)
target_sources(transform
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
)
target_link_libraries(transform
    PRIVATE
        glad glfw stb_image glm
)