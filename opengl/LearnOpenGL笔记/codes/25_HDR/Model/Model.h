#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <chrono>
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include "../Buffers/InstanceBuffer.h"
#include "RenderObject.h"
#include "../Textures/Texture2D.h"
#include "assimp/material.h"

class Model {
public:
    // self：被更新的 Model 自身（由调用方传入，避免捕获悬空 this）
    using Action = std::function<void(Model &self,
                                      std::chrono::steady_clock::time_point start_tp,
                                      std::chrono::duration<float> delta_time)>;

    Model() = default;
    Model(RenderObject object);
    Model(std::vector<RenderObject> objects);
    Model(const std::string_view &dir, const std::string_view &file,
          ShaderProgram render_shader = ShaderProgram{},
          ShaderProgram directional_shadow_shader = ShaderProgram{},
          ShaderProgram omni_shadow_shader = ShaderProgram{},
          bool flipUV = true);
    
    void setInstances(InstanceBuffer ibo);
    void setRenderShader(ShaderProgram shader);
    void setDirectionalShadowShader(ShaderProgram shader);
    void setOmniShadowShader(ShaderProgram shader);
    // 同时清空/设置两种阴影 shader（如灯具不投射阴影）
    void setShadowShaders(ShaderProgram directional, ShaderProgram omni);

    std::vector<RenderObject> objects_;
    std::optional<Action> update_action_;
private:
    void processNode(const aiNode *node, const aiScene *scene, const std::string_view &dir);
    RenderObject processMesh(const aiMesh *mesh, const aiScene *scene, const std::string_view &dir);
    std::vector<Texture> loadTextureFrom(const aiMaterial *material, const aiScene *scene, aiTextureType type,
                                         const std::string_view &dir, bool srgb = true);
    static std::map<std::string, Texture> s_loaded_textures;
};
