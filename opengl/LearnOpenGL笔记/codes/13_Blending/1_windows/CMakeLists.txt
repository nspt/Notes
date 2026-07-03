add_executable(window)
target_compile_features(window PRIVATE cxx_std_20)
target_sources(window
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
target_link_libraries(window
    PRIVATE
        glad glfw stb_image glm assimp
)