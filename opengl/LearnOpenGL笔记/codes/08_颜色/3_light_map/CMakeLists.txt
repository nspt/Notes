add_executable(light_map)
target_compile_features(light_map PRIVATE cxx_std_20)
target_sources(light_map
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
        Light.h
        Material.h
        RenderObject.h
)
target_link_libraries(light_map
    PRIVATE
        glad glfw stb_image glm
)