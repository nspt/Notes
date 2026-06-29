add_executable(depth_buffer)
target_compile_features(depth_buffer PRIVATE cxx_std_20)
target_sources(depth_buffer
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
)
target_link_libraries(depth_buffer
    PRIVATE
        glad glfw stb_image glm assimp
)