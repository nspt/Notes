add_executable(instance)
target_compile_features(instance PRIVATE cxx_std_20)
target_sources(instance
    PRIVATE
        main.cpp
        Mesh.h Mesh.cpp
        ShaderProgram.h ShaderProgram.cpp
        Texture.h Texture.cpp
        Texture2D.h Texture2D.cpp
        TextureCubeMap.h TextureCubeMap.cpp
        Camera.h Camera.cpp
        Buffers.h Buffers.cpp
        Light.h
        Material.h
        RenderObject.h
        Model.h Model.cpp
        FrameBuffer.h
)
target_link_libraries(instance
    PRIVATE
        glad glfw stb_image glm assimp
)