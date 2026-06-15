add_executable(multiple_lights)
target_compile_features(multiple_lights PRIVATE cxx_std_20)
target_sources(multiple_lights
    PRIVATE
        main.cpp
        Mesh.h
        Mesh.cpp
        ShaderProgram.h
        ShaderProgram.cpp
        Texture2D.h
        Texture2D.cpp
        Camera.h
        Camera.cpp
        Buffers.h
        Buffers.cpp
        Light.h
        Material.h
        RenderObject.h
)
target_link_libraries(multiple_lights
    PRIVATE
        glad glfw stb_image glm
)