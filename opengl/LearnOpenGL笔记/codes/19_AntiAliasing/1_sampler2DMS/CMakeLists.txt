add_executable(sampler2DMS)
target_compile_features(sampler2DMS PRIVATE cxx_std_20)
target_sources(sampler2DMS
    PRIVATE
        main.cpp
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
        RenderObject.h
        Model.h Model.cpp
        FrameBuffer.h
)
target_link_libraries(sampler2DMS
    PRIVATE
        glad glfw stb_image glm assimp
)