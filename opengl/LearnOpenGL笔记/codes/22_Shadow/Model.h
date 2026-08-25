#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <memory>
#include <map>
#include "Buffers.h"
#include "RenderObject.h"
#include "ShaderProgram.h"
#include "Texture2D.h"
#include "assimp/material.h"

class Model {
public:
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

    void render(ShaderProgram *program = nullptr);
    void renderDirectionalShadow();
    void renderOmniShadow();

    std::vector<RenderObject> objects_;
private:
    void processNode(const aiNode *node, const aiScene *scene, const std::string_view &dir);
    RenderObject processMesh(const aiMesh *mesh, const aiScene *scene, const std::string_view &dir);
    std::vector<Texture> loadTextureFrom(const aiMaterial *material, const aiScene *scene, aiTextureType type, const std::string_view &dir);
    static std::map<std::string, Texture> s_loaded_textures;
};