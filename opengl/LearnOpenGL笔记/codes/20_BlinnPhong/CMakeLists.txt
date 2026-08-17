add_executable(blinn_phong)
target_compile_features(blinn_phong PRIVATE cxx_std_20)
target_sources(blinn_phong
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
target_link_libraries(blinn_phong
    PRIVATE
        glad glfw stb_image glm assimp
)