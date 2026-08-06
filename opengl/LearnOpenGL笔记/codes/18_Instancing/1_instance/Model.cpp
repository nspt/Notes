#include "Model.h"
#include "Buffers.h"
#include "Material.h"
#include "RenderObject.h"
#include "Texture2D.h"
#include "assimp/material.h"
#include "assimp/scene.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <iostream>

using namespace std::string_literals;

std::map<std::string, std::weak_ptr<Texture2D>> Model::s_loaded_textures;

Model::Model(const std::string_view &dir, const std::string_view &file)
{
    std::string path{ dir };
    path.push_back('/');
    path += file;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path.c_str(),
        aiProcess_Triangulate | aiProcess_FlipUVs
    );

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error{ "ERROR::ASSIMP::"s + importer.GetErrorString() };
    }

    processNode(scene->mRootNode, scene, dir);
}

void Model::processNode(const aiNode *node, const aiScene *scene, const std::string_view &dir)
{
    // node->mTransformation 表示 Node 的变换，为简化示例，这里暂未处理 aiNode::mTransformation。
    // 若模型存在节点层级变换，应将父节点与当前节点的变换矩阵累乘后传递给子节点。
    // 当前实现忽略。
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        objects_.push_back(processMesh(mesh, scene, dir));			
    }
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, dir);
    }
}

std::shared_ptr<RenderObject> Model::processMesh(const aiMesh *mesh, const aiScene *scene, const std::string_view &dir)
{
    auto obj = std::make_shared<RenderObject>();

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    if (!mesh->HasTextureCoords(0))
        throw std::runtime_error{ "Mesh has no texture 0" };
    
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;
        vertex.texCoord.x = mesh->mTextureCoords[0][i].x; 
        vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        vertices.push_back(vertex);
    }
    // process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    obj->mesh_ = std::make_shared<Mesh>(
        VertexBuffer{ vertices },
        IndexBuffer{ indices },
        std::nullopt
    );

    obj->material_ = std::make_shared<Material>();
    // 若模型未提供 shininess，则使用默认值
    obj->material_->shininess_ = 32.0f;
    if(mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        obj->material_->diffuse_textures_ = loadTextureFrom(material, scene, aiTextureType_DIFFUSE, dir);
        obj->material_->specular_textures_ = loadTextureFrom(material, scene, aiTextureType_SPECULAR, dir);
        material->Get(AI_MATKEY_SHININESS, obj->material_->shininess_);
    }

    return obj;
}

std::vector<std::shared_ptr<Texture>> Model::loadTextureFrom(const aiMaterial *material, const aiScene *scene, aiTextureType type, const std::string_view &dir)
{
    std::string d{ dir };
    d.push_back('/');
    std::vector<std::shared_ptr<Texture>> textures;
    for(unsigned i = 0; i < material->GetTextureCount(type); ++i)
    {
        aiString str;
        material->GetTexture(type, i, &str);
        std::string path = d + str.C_Str();
        bool skip = false;
        auto iter = s_loaded_textures.find(path);
        if (iter != s_loaded_textures.end()) {
            if (auto p = iter->second.lock()) {
                skip = true;
                textures.push_back(p);
            } else {
                s_loaded_textures.erase(iter);
            }
        }
        if(!skip)
        {
            auto texture = std::make_shared<Texture2D>(path);
            textures.push_back(texture);
            s_loaded_textures.insert(
                std::pair<std::string, std::weak_ptr<Texture2D>>(path, texture)
            );
        }
    }

    return textures;
}