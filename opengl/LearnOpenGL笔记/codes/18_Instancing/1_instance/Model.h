#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <memory>
#include <map>
#include "RenderObject.h"
#include "ShaderProgram.h"
#include "Texture2D.h"
#include "assimp/material.h"

class Model {
public:
    Model(const std::string_view &dir, const std::string_view &file);
    std::vector<std::shared_ptr<RenderObject>> objects_;
    std::shared_ptr<ShaderProgram> program_;
private:
    void processNode(const aiNode *node, const aiScene *scene, const std::string_view &dir);
    std::shared_ptr<RenderObject> processMesh(const aiMesh *mesh, const aiScene *scene, const std::string_view &dir);
    std::vector<std::shared_ptr<Texture>> loadTextureFrom(const aiMaterial *material, const aiScene *scene, aiTextureType type, const std::string_view &dir);

    static std::map<std::string, std::weak_ptr<Texture2D>> s_loaded_textures;
};