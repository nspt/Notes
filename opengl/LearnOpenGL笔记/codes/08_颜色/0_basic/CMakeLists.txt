add_executable(color_basic)
target_compile_features(color_basic PRIVATE cxx_std_20)
target_sources(color_basic
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
)
target_link_libraries(color_basic
    PRIVATE
        glad glfw stb_image glm
)