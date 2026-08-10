#include "Model.h"
#include "Buffers.h"
#include "Material.h"
#include "RenderObject.h"
#include "ShaderProgram.h"
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

std::map<std::string, Texture> Model::s_loaded_textures;

Model::Model(const std::string_view &dir, const std::string_view &file, ShaderProgram shader_program, bool flipUV)
    : program_{ shader_program }
{
    std::string path{ dir };
    path.push_back('/');
    path += file;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path.c_str(),
        flipUV ? (aiProcess_Triangulate | aiProcess_FlipUVs) : aiProcess_Triangulate
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

RenderObject Model::processMesh(const aiMesh *mesh, const aiScene *scene, const std::string_view &dir)
{
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

    VertexBuffer vbo{ vertices };
    IndexBuffer ebo{ indices };

    Material material;
    // 若模型未提供 shininess，则使用默认值
    material.shininess_ = 32.0f;
    if(mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial *m = scene->mMaterials[mesh->mMaterialIndex];
        material.diffuse_textures_ = loadTextureFrom(m, scene, aiTextureType_DIFFUSE, dir);
        material.specular_textures_ = loadTextureFrom(m, scene, aiTextureType_SPECULAR, dir);
        m->Get(AI_MATKEY_SHININESS, material.shininess_);
    }

    return RenderObject{
        material, Mesh{ vbo, ebo }
    };
}

std::vector<Texture> Model::loadTextureFrom(const aiMaterial *material, const aiScene *scene, aiTextureType type, const std::string_view &dir)
{
    std::string d{ dir };
    d.push_back('/');
    std::vector<Texture> textures;
    for(unsigned i = 0; i < material->GetTextureCount(type); ++i)
    {
        aiString str;
        material->GetTexture(type, i, &str);
        std::string path = d + str.C_Str();
        if (auto iter = s_loaded_textures.find(path); iter != s_loaded_textures.end()) {
            textures.push_back(iter->second);
        } else {
            textures.push_back(Texture2D{ path });
            s_loaded_textures.insert_or_assign(path, textures.back());
        }
    }

    return textures;
}

void Model::setInstances(InstanceBuffer ibo)
{
    for (auto &obj : objects_) {
        obj.mesh_.setInstanceBuffer(ibo);
    }
}
