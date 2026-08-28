add_executable(normal_map)
target_compile_features(normal_map PRIVATE cxx_std_20)
target_sources(normal_map
    PRIVATE
        main.cpp
        Renderer.h Renderer.cpp
        Mesh.h Mesh.cpp
        ShaderProgram.h ShaderProgram.cpp
        Texture.h Texture.cpp
        Texture2D.h Texture2D.cpp
        Texture2DMS.h Texture2DMS.cpp
        TextureCubeMap.h TextureCubeMap.cpp
        Camera.h Camera.cpp
        Buffers.h Buffers.cpp
        Light.h
        Material.h
        RenderState.h
        RenderObject.h
        Model.h Model.cpp
        FrameBuffer.h
)
target_link_libraries(normal_map
    PRIVATE
        glad glfw stb_image glm assimp
)