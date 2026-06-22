add_executable(model_load)
target_compile_features(model_load PRIVATE cxx_std_20)
target_sources(model_load
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
target_link_libraries(model_load
    PRIVATE
        glad glfw stb_image glm assimp
)