add_executable(fps_camera)
target_compile_features(fps_camera PRIVATE cxx_std_20)
target_sources(fps_camera
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
)
target_link_libraries(fps_camera
    PRIVATE
        glad glfw stb_image glm
)