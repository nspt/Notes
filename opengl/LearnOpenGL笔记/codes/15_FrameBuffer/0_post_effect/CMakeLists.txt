add_executable(post_effect)
target_compile_features(post_effect PRIVATE cxx_std_20)
target_sources(post_effect
    PRIVATE
        main.cpp
        Mesh.h Mesh.cpp
        ShaderProgram.h ShaderProgram.cpp
        Texture2D.h Texture2D.cpp
        Camera.h Camera.cpp
        Buffers.h Buffers.cpp
        Light.h
        Material.h
        RenderObject.h
        Model.h Model.cpp
        FrameBuffer.h
)
target_link_libraries(post_effect
    PRIVATE
        glad glfw stb_image glm assimp
)