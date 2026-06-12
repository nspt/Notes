add_executable(material_basic)
target_compile_features(material_basic PRIVATE cxx_std_20)
target_sources(material_basic
    PRIVATE
        main.cpp
        Mesh.cpp
        ShaderProgram.cpp
        Texture2D.cpp
        Camera.cpp
        Light.h
        Material.h
        RenderObject.h
)
target_link_libraries(material_basic
    PRIVATE
        glad glfw stb_image glm
)