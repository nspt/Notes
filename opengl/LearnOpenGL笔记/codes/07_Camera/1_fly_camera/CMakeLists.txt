add_executable(fly_camera)
target_compile_features(fly_camera PRIVATE cxx_std_20)
target_sources(fly_camera
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
)
target_link_libraries(fly_camera
    PRIVATE
        glad glfw stb_image glm
)