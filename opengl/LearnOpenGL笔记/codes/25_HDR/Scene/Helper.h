#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../Buffers/IndexBuffer.h"
#include "../Buffers/InstanceBuffer.h"
#include "../Buffers/VertexBuffer.h"
#include "../Model/Material.h"
#include "../Model/Mesh.h"
#include "../Model/Model.h"
#include "../Model/RenderObject.h"
#include "Light.h"
#include "../Renderer/PipelineState.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/ShaderProgram.h"
#include "Scene.h"
#include "../Textures/Texture2D.h"
#include "../Textures/TextureCubeMap.h"


inline Renderer::ShaderMap loadShaders(const std::string &resourceDir, const std::string &shaderSubDir)
{
    Renderer::ShaderMap shaders;
    const auto shaderDir = std::filesystem::path{ resourceDir } / "shaders" / shaderSubDir;
    auto shaderPath = [&](const std::string &file) {
        return shaderDir / file;
    };
    auto add = [&](const std::string &name,
                   const std::filesystem::path &vert,
                   const std::filesystem::path &frag,
                   const std::filesystem::path &geom = {}) {
        shaders.emplace(name, ShaderProgram{ vert, frag, geom });
    };

    add("lit",
        shaderPath("lit.vert"),
        shaderPath("lit.frag"));
    shaders.at("lit").setUniformBlockBinding("LightData", 0);
    shaders.at("lit").setUniformBlockBinding("CamData", 1);

    add("shadow",
        shaderPath("shadow.vert"),
        shaderPath("shadow.frag"));

    add("cube_shadow",
        shaderPath("cube_shadow.vert"),
        shaderPath("cube_shadow.frag"),
        shaderPath("cube_shadow.geom"));

    add("visual_normal",
        shaderPath("visual_normal.vert"),
        shaderPath("visual_normal.frag"),
        shaderPath("visual_normal.geom"));
    shaders.at("visual_normal").setUniformBlockBinding("CamData", 1);
    shaders.at("visual_normal").setVec3("normal_color", glm::vec3{ 0, 1, 0 });

    add("explode",
        shaderPath("explode.vert"),
        shaderPath("explode.frag"),
        shaderPath("explode.geom"));
    shaders.at("explode").setUniformBlockBinding("LightData", 0);
    shaders.at("explode").setUniformBlockBinding("CamData", 1);

    add("explode_shadow",
        shaderPath("shadow_explode.vert"),
        shaderPath("shadow_explode.frag"),
        shaderPath("shadow_explode.geom"));

    add("skybox",
        shaderPath("skybox.vert"),
        shaderPath("skybox.frag"));
    shaders.at("skybox").setUniformBlockBinding("CamData", 1);
    shaders.at("skybox").setInt("skybox", 0);

    add("blur",
        shaderPath("blur.vert"),
        shaderPath("blur.frag"));

    add("composite",
        shaderPath("composite.vert"),
        shaderPath("composite.frag"));

    add("quad",
        shaderPath("quad.vert"),
        shaderPath("quad.frag"));
    return shaders;
}

inline Mesh createQuadMesh(
    InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} },
    bool include_inner_faces = true)
{
    // 外侧：朝 +Z；内侧另建顶点，法线翻转
    static const std::vector<Vertex> vertices_outer = {
        // position, texCoord, normal, tangent（+Z 面：u 沿 +X）
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
    };
    static const std::vector<std::uint32_t> indices_outer = {
        0, 1, 2,
        0, 2, 3,
    };

    static const std::vector<Vertex> vertices_with_inner = [] {
        constexpr float inset = 0.002f;
        std::vector<Vertex> verts = vertices_outer;
        verts.reserve(vertices_outer.size() * 2);
        for (const Vertex &v : vertices_outer) {
            verts.push_back(Vertex{
                v.position - v.normal * inset,
                v.texCoord,
                -v.normal,
                -v.tangent
            });
        }
        return verts;
    }();
    static const std::vector<std::uint32_t> indices_with_inner = [] {
        std::vector<std::uint32_t> idx = indices_outer;
        const std::uint32_t base = static_cast<std::uint32_t>(vertices_outer.size());
        for (size_t i = 0; i + 2 < indices_outer.size(); i += 3) {
            idx.push_back(indices_outer[i] + base);
            idx.push_back(indices_outer[i + 2] + base);
            idx.push_back(indices_outer[i + 1] + base);
        }
        return idx;
    }();

    static VertexBuffer vbo_outer{ vertices_outer };
    static IndexBuffer ebo_outer{ indices_outer };
    static VertexBuffer vbo_with_inner{ vertices_with_inner };
    static IndexBuffer ebo_with_inner{ indices_with_inner };

    if (include_inner_faces) {
        return Mesh{ vbo_with_inner, ebo_with_inner, std::move(ibo) };
    }
    return Mesh{ vbo_outer, ebo_outer, std::move(ibo) };
}

inline Mesh createCubeMesh(
    InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} },
    bool include_inner_faces = true)
{
    const float x = 1.0f, y = 1.0f, z = 1.0f;
    static const std::vector<Vertex> vertices_outer = {
        // +Z
        { { -x, -y, z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { x, -y, z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { x, y, z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -x, y, z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        // -Z
        { { x, -y, -z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -x, -y, -z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -x, y, -z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } },
        { { x, y, -z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f } },
        // -X
        { { -x, -y, -z }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -x, -y, z }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -x, y, z }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -x, y, -z }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        // +X
        { { x, -y, z }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { { x, -y, -z }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { { x, y, -z }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { { x, y, z }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        // +Y
        { { -x, y, z }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { x, y, z }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { x, y, -z }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -x, y, -z }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        // -Y
        { { x, -y, z }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -x, -y, z }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -x, -y, -z }, { 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },
        { { x, -y, -z }, { 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },
    };
    static const std::vector<std::uint32_t> indices_outer = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    // 内侧面：独立顶点 + 翻转法线；略向中心收缩，避免与外侧共�?z-fighting
    static const std::vector<Vertex> vertices_with_inner = [] {
        constexpr float inset = 0.998f;
        std::vector<Vertex> verts = vertices_outer;
        verts.reserve(vertices_outer.size() * 2);
        for (const Vertex &v : vertices_outer) {
            verts.push_back(Vertex{ v.position * inset, v.texCoord, -v.normal, -v.tangent });
        }
        return verts;
    }();
    static const std::vector<std::uint32_t> indices_with_inner = [] {
        std::vector<std::uint32_t> idx = indices_outer;
        const std::uint32_t base = static_cast<std::uint32_t>(vertices_outer.size());
        for (size_t i = 0; i + 2 < indices_outer.size(); i += 3) {
            idx.push_back(indices_outer[i] + base);
            idx.push_back(indices_outer[i + 2] + base);
            idx.push_back(indices_outer[i + 1] + base);
        }
        return idx;
    }();

    static VertexBuffer vbo_outer{ vertices_outer };
    static IndexBuffer ebo_outer{ indices_outer };
    static VertexBuffer vbo_with_inner{ vertices_with_inner };
    static IndexBuffer ebo_with_inner{ indices_with_inner };

    if (include_inner_faces) {
        return Mesh{ vbo_with_inner, ebo_with_inner, std::move(ibo) };
    }
    return Mesh{ vbo_outer, ebo_outer, std::move(ibo) };
}

inline Model createGrass(const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/grass.png");
    material.diffuse_->setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    return RenderObject{
        material, createQuadMesh()
    };
}

inline Model createGlassWindow(const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/window.png");
    material.diffuse_->setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    RenderObject gw{ material, createQuadMesh() };
    gw.pipeline_state_.blend_ = true;
    return gw;
}


inline Model createBrickwallCube(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/brickwall.jpg");
    material.normal_ = Texture2D(resourceDir + "/textures/brickwall_normal.jpg", true, false);
    material.shininess_ = 32.0f;

    return Model{ RenderObject{
        material, createCubeMesh(),
        r.shader("lit"), r.shader("shadow"), r.shader("cube_shadow")
    } };
}

inline Model createWoodCube(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/wood.png");
    material.shininess_ = 32.0f;

    return Model{ RenderObject{
        material, createCubeMesh(),
        r.shader("lit"), r.shader("shadow"), r.shader("cube_shadow")
    } };
}

inline Model createContainerCube(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/container2.png");
    material.specular_ = Texture2D(resourceDir + "/textures/container2_specular.png");
    material.shininess_ = 32.0f;

    return Model{ RenderObject{
        material, createCubeMesh(),
        r.shader("lit"), r.shader("shadow"), r.shader("cube_shadow")
    } };
}

inline Model createOutlineModel(Model &model, GLint stencil_ref = 1,
                                float scale_factor = 1.05f,
                                glm::vec3 outline_color = { 0.0f, 0.0f, 1.0f })
{
    Model outline = model;

    for (auto &obj : model.objects_) {
        obj.pipeline_state_.stencil_test_ = true;
        obj.pipeline_state_.stencil_func_ = GL_ALWAYS;
        obj.pipeline_state_.stencil_ref_ = stencil_ref;
        obj.pipeline_state_.stencil_mask_ = 0xff;
        obj.pipeline_state_.stencil_sfail_ = GL_KEEP;
        obj.pipeline_state_.stencil_dpfail_ = GL_KEEP;
        obj.pipeline_state_.stencil_dppass_ = GL_REPLACE;
        obj.pipeline_state_.stencil_write_mask_ = 0xff;
    }

    for (auto &obj : outline.objects_) {
        obj.pipeline_state_.stencil_test_ = true;
        obj.pipeline_state_.stencil_func_ = GL_NOTEQUAL;
        obj.pipeline_state_.stencil_ref_ = stencil_ref;
        obj.pipeline_state_.stencil_mask_ = 0xff;
        obj.pipeline_state_.stencil_sfail_ = GL_KEEP;
        obj.pipeline_state_.stencil_dpfail_ = GL_KEEP;
        obj.pipeline_state_.stencil_dppass_ = GL_KEEP;
        obj.pipeline_state_.stencil_write_mask_ = 0x00;

        auto instances = obj.mesh_.instanceBuffer().data();
        for (auto &instance : instances) {
            instance.scale_ *= scale_factor;
        }
        obj.mesh_.setInstanceBuffer(InstanceBuffer{ std::move(instances) });
        obj.material_.pure_color_ = true;
        obj.material_.color_ = outline_color;
    }

    return outline;
}

// 绕轴旋转：求点绕「过 pivot、方向为 axis 的轴」旋转 angle_rad 后的世界坐标。
// 流程：orbit_offset（轴局部）→ 世界偏移 → 绕轴旋转 → 加到 pivot。
//
// angle_rad    旋转角（弧度）。0 时结果为 pivot + 未旋转的局部偏移。
// axis         旋转轴方向（世界空间；不必单位化，内部会 normalize）。
// pivot        轴上的一点，也是轨道中心（世界空间）。最终位置 = pivot + 旋转后的偏移。
// orbit_offset 相对该轴的局部偏移（把 axis 当作局部 Y）：
//              - .x：垂直于轴的横向（局部 u）
//              - .y：沿轴方向（局部 n，与 axis 同向）
//              - .z：另一垂直方向（局部 v = n × u）
//              例：orbit_offset = (0, 0, 5) 表示在垂直于轴的平面上、距轴 5；
//              再改变 angle_rad，即绕 pivot 在该平面上转圈。
inline glm::vec3 rotateAroundAxis(
    float angle_rad,
    const glm::vec3 &axis = glm::vec3{ 1.0f, 1.0f, 0.0f },
    const glm::vec3 &pivot = glm::vec3{ 0.0f },
    const glm::vec3 &orbit_offset = glm::vec3{ 0.0f, 0.0f, 5.0f })
{
    const glm::vec3 n = glm::normalize(axis);

    const glm::vec3 helper = (std::abs(n.y) < 0.99f) ? glm::vec3{ 0.0f, 1.0f, 0.0f }
                                                     : glm::vec3{ 1.0f, 0.0f, 0.0f };
    const glm::vec3 u = glm::normalize(glm::cross(helper, n));
    const glm::vec3 v = glm::cross(n, u);
    const glm::vec3 local_offset = u * orbit_offset.x + n * orbit_offset.y + v * orbit_offset.z;

    const glm::mat4 rot = glm::rotate(glm::mat4{ 1.0f }, angle_rad, n);
    const glm::vec3 rotated_offset = glm::vec3(rot * glm::vec4{ local_offset, 0.0f });
    return pivot + rotated_offset;
}
