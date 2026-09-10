#include "Model.h"
#include "../Buffers/IndexBuffer.h"
#include "../Buffers/InstanceBuffer.h"
#include "../Buffers/VertexBuffer.h"
#include "Material.h"
#include "RenderObject.h"
#include "../Renderer/ShaderProgram.h"
#include "../Textures/Texture2D.h"
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

Model::Model(RenderObject object)
    : objects_{ std::move(object) }
{}

Model::Model(std::vector<RenderObject> objects)
    : objects_{ std::move(objects) }
{}

Model::Model(const std::string_view &dir, const std::string_view &file,
             ShaderProgram render_shader,
             ShaderProgram directional_shadow_shader,
             ShaderProgram omni_shadow_shader,
             bool flipUV)
{
    std::string path{ dir };
    path.push_back('/');
    path += file;

    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
    if (flipUV) {
        flags |= aiProcess_FlipUVs;
    }
    const aiScene *scene = importer.ReadFile(path.c_str(), flags);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error{ "ERROR::ASSIMP::"s + importer.GetErrorString() };
    }

    processNode(scene->mRootNode, scene, dir);

    for (auto &obj : objects_) {
        obj.render_shader_ = render_shader;
        obj.directional_shadow_shader_ = directional_shadow_shader;
        obj.omni_shadow_shader_ = omni_shadow_shader;
    }
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
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent.x = mesh->mTangents[i].x;
            vertex.tangent.y = mesh->mTangents[i].y;
            vertex.tangent.z = mesh->mTangents[i].z;
        } else {
            vertex.tangent = glm::vec3{ 0.0f, 0.0f, 1.0f };
        }
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
    if(mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial *m = scene->mMaterials[mesh->mMaterialIndex];
        if (auto textures = loadTextureFrom(m, scene, aiTextureType_DIFFUSE, dir); !textures.empty()) {
            material.diffuse_ = std::move(textures.front());
        }
        if (auto textures = loadTextureFrom(m, scene, aiTextureType_SPECULAR, dir); !textures.empty()) {
            material.specular_ = std::move(textures.front());
        }
        if (auto textures = loadTextureFrom(m, scene, aiTextureType_NORMALS, dir, false); !textures.empty()) {
            material.normal_ = std::move(textures.front());
        }
        if (auto textures = loadTextureFrom(m, scene, aiTextureType_HEIGHT, dir, false); !textures.empty()) {
            material.height_ = std::move(textures.front());
        }
        m->Get(AI_MATKEY_SHININESS, material.shininess_);
    }

    return RenderObject{
        material, Mesh{ vbo, ebo }
    };
}

std::vector<Texture> Model::loadTextureFrom(const aiMaterial *material, const aiScene *scene, aiTextureType type,
                                            const std::string_view &dir, bool srgb)
{
    std::string d{ dir };
    d.push_back('/');
    std::vector<Texture> textures;
    for(unsigned i = 0; i < material->GetTextureCount(type); ++i)
    {
        aiString str;
        material->GetTexture(type, i, &str);
        std::string path = d + str.C_Str();
        // 缓存 key 区分 sRGB，避�?diffuse/normal 共用路径时格式冲�?
    const std::string cache_key = path + (srgb ? "|srgb" : "|linear");
        if (auto iter = s_loaded_textures.find(cache_key); iter != s_loaded_textures.end()) {
            textures.push_back(iter->second);
        } else {
            textures.push_back(Texture2D{ path, true, srgb });
            s_loaded_textures.insert_or_assign(cache_key, textures.back());
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

void Model::setRenderShader(ShaderProgram shader)
{
    for (auto &obj : objects_) {
        obj.render_shader_ = shader;
    }
}

void Model::setDirectionalShadowShader(ShaderProgram shader)
{
    for (auto &obj : objects_) {
        obj.directional_shadow_shader_ = shader;
    }
}

void Model::setOmniShadowShader(ShaderProgram shader)
{
    for (auto &obj : objects_) {
        obj.omni_shadow_shader_ = shader;
    }
}

void Model::setShadowShaders(ShaderProgram directional, ShaderProgram omni)
{
    for (auto &obj : objects_) {
        obj.directional_shadow_shader_ = directional;
        obj.omni_shadow_shader_ = omni;
    }
}
