add_executable(mirror)
target_compile_features(mirror PRIVATE cxx_std_20)
target_sources(mirror
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
target_link_libraries(mirror
    PRIVATE
        glad glfw stb_image glm assimp
)