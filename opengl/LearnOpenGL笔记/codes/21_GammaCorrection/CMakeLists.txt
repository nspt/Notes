add_executable(gamma_correction)
target_compile_features(gamma_correction PRIVATE cxx_std_20)
target_sources(gamma_correction
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
target_link_libraries(gamma_correction
    PRIVATE
        glad glfw stb_image glm assimp
)