add_executable(auto_rotate)
target_compile_features(auto_rotate PRIVATE cxx_std_20)
target_sources(auto_rotate
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
)
target_link_libraries(auto_rotate
    PRIVATE
        glad glfw stb_image glm
)