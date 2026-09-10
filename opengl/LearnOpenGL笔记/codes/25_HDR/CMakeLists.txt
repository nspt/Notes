add_executable(hdr)
target_compile_features(hdr PRIVATE cxx_std_20)

target_sources(hdr
    PRIVATE
        main.cpp
        GLFWWin.h GLFWWin.cpp
        Scene/Scene.h
        Scene/Helper.h
        Scene/Camera.h Scene/Camera.cpp
        Scene/Light.h
        Scene/Shadow.h

        Buffers/Buffer.h Buffers/Buffer.cpp
        Buffers/VertexBuffer.h Buffers/VertexBuffer.cpp
        Buffers/IndexBuffer.h Buffers/IndexBuffer.cpp
        Buffers/InstanceBuffer.h Buffers/InstanceBuffer.cpp
        Buffers/UniformBuffer.h Buffers/UniformBuffer.cpp

        FrameBuffer/FrameBuffer.h FrameBuffer/FrameBuffer.cpp
        FrameBuffer/RenderBuffer.h FrameBuffer/RenderBuffer.cpp
        FrameBuffer/RenderBufferMS.h FrameBuffer/RenderBufferMS.cpp

        Model/Material.h
        Model/Mesh.h Model/Mesh.cpp
        Model/Model.h Model/Model.cpp
        Model/RenderObject.h

        Renderer/Renderer.h Renderer/Renderer.cpp
        Renderer/PipelineState.h Renderer/PipelineState.cpp
        Renderer/ShaderProgram.h Renderer/ShaderProgram.cpp

        Textures/Texture.h Textures/Texture.cpp
        Textures/Texture2D.h Textures/Texture2D.cpp
        Textures/Texture2DMS.h Textures/Texture2DMS.cpp
        Textures/TextureCubeMap.h Textures/TextureCubeMap.cpp
)
target_link_libraries(hdr
    PRIVATE
        glad glfw stb_image glm assimp
)
