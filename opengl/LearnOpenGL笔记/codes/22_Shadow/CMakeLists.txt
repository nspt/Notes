add_executable(shadow)
target_compile_features(shadow PRIVATE cxx_std_20)
target_sources(shadow
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
target_link_libraries(shadow
    PRIVATE
        glad glfw stb_image glm assimp
)