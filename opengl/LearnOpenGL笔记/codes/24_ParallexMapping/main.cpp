#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Buffers.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"
#include "RenderObject.h"
#include "RenderState.h"
#include "Renderer.h"
#include "Texture2D.h"
#include "TextureCubeMap.h"

// ---------------------------------------------------------------------------
// 场景几何 / 物体创建（业务）
// ---------------------------------------------------------------------------

namespace {

std::vector<Model *> point_light_markers;

constexpr float kPointLightMarkerScale = 0.2f;

void syncPointLightMarkers(const LightData &lights)
{
    for (int i = 0; i < lights.counts.y && i < static_cast<int>(point_light_markers.size()); ++i) {
        InstanceBuffer::InstanceData marker;
        marker.translation_ = glm::vec3(lights.point[i].pos_);
        marker.scale_ = glm::vec3{ kPointLightMarkerScale };
        point_light_markers[i]->objects_[0].mesh_.setInstanceBuffer(InstanceBuffer{ marker });
    }
}

} // namespace

Mesh createQuadMesh(
    InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} },
    bool include_inner_faces = true)
{
    // 外侧：朝 +Z；内侧另建顶点，法线翻转到 -Z
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

    // 沿 -normal 微偏移，避免与外侧共面 z-fighting（四边形在 z=0，不能靠 position*inset）
    static const std::vector<Vertex> vertices_with_inner = [] {
        constexpr float inset = 0.002f;
        std::vector<Vertex> verts = vertices_outer;
        verts.reserve(vertices_outer.size() * 2);
        for (const Vertex &v : vertices_outer) {
            verts.push_back(Vertex{
                v.position - v.normal * inset,
                v.texCoord,
                -v.normal,
                v.tangent
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

Mesh createCubeMesh(
    InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} },
    bool include_inner_faces = true)
{
    const float x = 0.5f, y = 0.5f, z = 0.5f;
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

    // 内侧面：独立顶点 + 翻转法线；略向中心收缩，避免与外侧共面 z-fighting
    static const std::vector<Vertex> vertices_with_inner = [] {
        constexpr float inset = 0.998f;
        std::vector<Vertex> verts = vertices_outer;
        verts.reserve(vertices_outer.size() * 2);
        for (const Vertex &v : vertices_outer) {
            verts.push_back(Vertex{ v.position * inset, v.texCoord, -v.normal, v.tangent });
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

RenderObject createGrasses(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/grass.png");
    material.diffuse_->setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    std::vector<InstanceBuffer::InstanceData> grasses {
        { .translation_ = { 7.0f,  1.0f,  -4.0f } },
        { .translation_ = { 0.0f,  1.0f,  -4.0f } },
        { .translation_ = { -7.0f, 1.0f, -3.0f } },
        { .translation_ = { -7.0f, 1.0f, 0.0f } },
        { .translation_ = { 7.0f,  1.0f,  0.0f } },
        { .translation_ = { -7.0f, 1.0f, 3.0f } },
        { .translation_ = { 0.0f,  1.0f, 3.0f } },
        { .translation_ = { 7.0f,  1.0f,  4.0f } },
    };
    return RenderObject{
        material, createQuadMesh(InstanceBuffer{ std::move(grasses) }),
        r.shader("general"), r.shader("shadow"), r.shader("cube_shadow")
    };
}

std::vector<RenderObject> createGlasses(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/window.png");
    material.diffuse_->setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    std::vector<InstanceBuffer::InstanceData> instances {
        { .translation_ = { 7.0f,  1.0f,  -5.0f } },
        { .translation_ = { 0.0f,  1.0f,  -5.0f } },
        { .translation_ = { -7.0f, 1.0f, -4.0f } },
        { .translation_ = { -7.0f, 1.0f, 1.0f } },
        { .translation_ = { 7.0f,  1.0f,  1.0f } },
        { .translation_ = { -7.0f, 1.0f, 4.0f } },
        { .translation_ = { 0.0f,  1.0f, 4.0f } },
        { .translation_ = { 7.0f,  1.0f,  5.0f } },
    };
    std::vector<RenderObject> glasses;
    glasses.reserve(instances.size());
    for (auto &instance : instances) {
        glasses.push_back(RenderObject{
            material, createQuadMesh(instance),
            r.shader("general"), r.shader("shadow"), r.shader("cube_shadow")
        });
        glasses.back().render_state_.blend_ = true;
    }
    return glasses;
}

RenderObject createPlatform(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/container2.png");
    material.specular_ = Texture2D(resourceDir + "/textures/container2_specular.png");
    material.shininess_ = 64.0f;

    InstanceBuffer::InstanceData platform;
    platform.translation_ = glm::vec3{ 0.0f, -50.0f, 0.0f };
    platform.scale_ = glm::vec3{ 100.0f };

    return RenderObject{
        material, createCubeMesh(platform),
        r.shader("general"), r.shader("shadow"), r.shader("cube_shadow")
    };
}

void addPlatformInterior(Renderer &r, const std::string &resourceDir, const LightData &lights)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/brickwall.jpg");
    material.normal_ = Texture2D(resourceDir + "/textures/brickwall_normal.jpg", true, false);
    material.shininess_ = 32.0f;

    const glm::vec3 room_center{ 0.0f, -50.0f, 0.0f };
    std::vector<InstanceBuffer::InstanceData> cubes {
        { .translation_ = room_center + glm::vec3{  35.0f, -5.0f,  0.0f }, .scale_ = { 2.0f, 2.0f, 2.0f } },
        { .translation_ = room_center + glm::vec3{ -38.0f,  8.0f,  20.0f }, .scale_ = { 1.5f, 3.0f, 1.5f } },
        { .translation_ = room_center + glm::vec3{  25.0f,-30.0f, -35.0f }, .scale_ = { 2.5f, 1.5f, 2.5f } },
        { .translation_ = room_center + glm::vec3{ -30.0f,-20.0f, -38.0f }, .scale_ = { 1.0f, 1.0f, 4.0f } },
        { .translation_ = room_center + glm::vec3{   0.0f,-40.0f,  36.0f }, .scale_ = { 3.0f, 1.0f, 3.0f } },
        { .translation_ = room_center + glm::vec3{  40.0f,  0.0f,  32.0f }, .scale_ = { 1.2f, 1.2f, 1.2f } },
    };
    for (auto &cube : cubes) {
        r.objects().push_back(RenderObject{
            material, createCubeMesh(cube, false),
            r.shader("general"), r.shader("shadow"), r.shader("cube_shadow")
        });
    }

    for (int i = 0; i < lights.counts.y; ++i) {
        Material marker_mat;
        marker_mat.pure_color_ = true;
        marker_mat.color_ = glm::vec3{ 1.0f };
        InstanceBuffer::InstanceData marker;
        marker.translation_ = glm::vec3(lights.point[i].pos_);
        marker.scale_ = glm::vec3{ 0.5f };
        r.objects().push_back(RenderObject{
            marker_mat, createCubeMesh(marker, false),
            r.shader("general"), ShaderProgram{}, ShaderProgram{}
        });
    }
}

std::vector<RenderObject> createCubes(Renderer &r, const std::string &resourceDir)
{
    Material material;
    material.diffuse_ = Texture2D(resourceDir + "/textures/brickwall.jpg");
    material.normal_ = Texture2D(resourceDir + "/textures/brickwall_normal.jpg", true, false);
    material.shininess_ = 32.0f;

    std::vector<InstanceBuffer::InstanceData> cubes {
        { .translation_ = { 0.0f,  1.0f,  0.0f }, .scale_ = { 2, 2, 2 } },
        { .translation_ = { 10.0f,  0.5f,  0.0f } },
        { .translation_ = { 10.0f,  1.7f,  0.0f } }
    };

    std::vector<RenderObject> result;
    for (auto &cube : cubes) {
        result.emplace_back(
            material, createCubeMesh(cube),
            r.shader("general"), r.shader("shadow"), r.shader("cube_shadow")
        );
    }
    return result;
}

std::vector<RenderObject> createOutlineCubes(Renderer &r, const std::string &resourceDir)
{
    auto setupStencilMaskWriter = [](RenderObject &obj, GLint ref = 1) {
        obj.render_state_.stencil_test_ = true;
        obj.render_state_.stencil_func_ = GL_ALWAYS;
        obj.render_state_.stencil_ref_ = ref;
        obj.render_state_.stencil_mask_ = 0xff;
        obj.render_state_.stencil_sfail_ = GL_KEEP;
        obj.render_state_.stencil_dpfail_ = GL_KEEP;
        obj.render_state_.stencil_dppass_ = GL_REPLACE;
        obj.render_state_.stencil_write_mask_ = 0xff;
    };
    auto setupOutline = [](RenderObject &obj, GLint ref = 1) {
        obj.render_state_.stencil_test_ = true;
        obj.render_state_.stencil_func_ = GL_NOTEQUAL;
        obj.render_state_.stencil_ref_ = ref;
        obj.render_state_.stencil_mask_ = 0xff;
        obj.render_state_.stencil_sfail_ = GL_KEEP;
        obj.render_state_.stencil_dpfail_ = GL_KEEP;
        obj.render_state_.stencil_dppass_ = GL_KEEP;
        obj.render_state_.stencil_write_mask_ = 0x00;
    };
    auto cubes = createCubes(r, resourceDir);
    std::vector<RenderObject> result;
    result.reserve(cubes.size() * 2);
    for (auto &cube : cubes) {
        setupStencilMaskWriter(cube);
        result.push_back(cube);
    }
    for (auto &cube : cubes) {
        setupOutline(cube);
        auto instances = cube.mesh_.instanceBuffer().data();
        for (auto &instance : instances) {
            instance.scale_ *= 1.05f;
        }
        cube.mesh_.setInstanceBuffer(InstanceBuffer{ std::move(instances) });
        cube.material_.pure_color_ = true;
        cube.material_.color_ = glm::vec3{ 0.0, 0.0, 1.0 };
        result.push_back(std::move(cube));
    }
    return result;
}

void createScene(Renderer &r, const std::string &resourceDir)
{
    auto &general = r.shader("general");
    auto &shadow = r.shader("shadow");
    auto &cube_shadow = r.shader("cube_shadow");
    const auto &lights = r.lights();

    RenderObject no_normal_wall;
    no_normal_wall.render_shader_ = general;
    no_normal_wall.mesh_ = createQuadMesh(InstanceBuffer::InstanceData{ .translation_ = glm::vec3{ -2.2, 0, -0.5 } });
    no_normal_wall.material_.diffuse_ = Texture2D(resourceDir + "/textures/bricks2.jpg");
    no_normal_wall.material_.specular_ = *no_normal_wall.material_.diffuse_;
    r.objects().push_back(no_normal_wall);

    RenderObject normal_wall = no_normal_wall;
    normal_wall.material_.normal_ = Texture2D(resourceDir + "/textures/bricks2_normal.jpg", true, false);
    normal_wall.mesh_.setInstanceBuffer(InstanceBuffer::InstanceData{ .translation_ = glm::vec3{ 0.0, 0, -0.5 } });
    r.objects().push_back(normal_wall);

    RenderObject parallax_wall = normal_wall;
    parallax_wall.material_.height_ = Texture2D(resourceDir + "/textures/bricks2_disp.jpg", true, false);
    parallax_wall.material_.height_scale_ = 0.05f;
    parallax_wall.mesh_.setInstanceBuffer(InstanceBuffer::InstanceData{ .translation_ = glm::vec3{ 2.2, 0, -0.5 } });
    r.objects().push_back(parallax_wall);

    for (int i = 0; i < lights.counts.y; ++i) {
        Material marker_mat;
        marker_mat.pure_color_ = true;
        marker_mat.color_ = glm::vec3{ 1.0f };
        InstanceBuffer::InstanceData marker;
        marker.translation_ = glm::vec3(lights.point[i].pos_);
        marker.scale_ = glm::vec3{ kPointLightMarkerScale };
        r.objects().push_back(RenderObject{
            marker_mat, createCubeMesh(marker, false),
            general, ShaderProgram{}, ShaderProgram{}
        });
        point_light_markers.push_back(&r.objects().back());
    }

    // r.outlineObjects().push_back(createOutlineCubes(r, resourceDir));
    // r.objects().push_back(createPlatform(r, resourceDir));
    // addPlatformInterior(r, resourceDir, lights);

    // Model flashlight{ resourceDir + "/model/flash_light", "Flashlight.obj",
    //                   general, shadow, cube_shadow, false };
    // for (int i = 0; i < lights.counts.z; ++i) {
    //     auto &spot{ lights.spot[i] };
    //     InstanceBuffer::InstanceData instance;
    //     instance.translation_ = spot.pos_;

    //     glm::vec3 direction{ spot.direction_inner_ };
    //     float yaw = atan2(direction.x, direction.z);
    //     float horizontal = std::hypot(direction.x, direction.z);
    //     float pitch = atan2(-direction.y, horizontal);
    //     instance.rotation_ = glm::angleAxis(yaw, glm::vec3{ 0, 1, 0 })
    //         * glm::angleAxis(pitch, glm::vec3{ 1, 0, 0 });
    //     instance.scale_ = glm::vec3{ 0.3f };

    //     r.objects().push_back(flashlight);
    //     r.objects().back().setInstances(InstanceBuffer{ instance });
    //     r.objects().back().setShadowShaders(ShaderProgram{}, ShaderProgram{});
    // }

    // Model backpack{ resourceDir + "/model/backpack", "backpack.obj",
    //                 general, shadow, cube_shadow };
    // backpack.setInstances(InstanceBuffer::InstanceData{ .translation_ = { -5, 2, 0 } });
    // r.objects().push_back(backpack);

    // backpack.setInstances(InstanceBuffer::InstanceData{ .translation_ = { -5, 2, -5 } });
    // r.objects().push_back(backpack);

    // backpack.setRenderShader(r.shader("visual_normal"));
    // r.objects().push_back(backpack);

    // backpack.setRenderShader(r.shader("explode"));
    // backpack.setDirectionalShadowShader(r.shader("explode_shadow"));
    // backpack.setInstances(InstanceBuffer::InstanceData{ .translation_ = { 0, 4, -5 } });
    // r.objects().push_back(backpack);

    // const glm::vec3 planet_pos{ 10, 30, 0 };
    // r.objects().emplace_back(resourceDir + "/model/planet", "planet.obj",
    //                          general, shadow, cube_shadow);
    // r.objects().back().setInstances(InstanceBuffer::InstanceData{ .translation_ = planet_pos });

    // r.objects().push_back(createGrasses(r, resourceDir));

    // RenderObject skybox_obj{ Material{}, createCubeMesh() };
    // skybox_obj.render_shader_ = r.shader("skybox");
    // skybox_obj.render_state_.cull_face_ = false;
    // skybox_obj.render_state_.depth_func_ = GL_LEQUAL;
    // skybox_obj.material_.skybox_ = TextureCubeMap{
    //     {
    //         resourceDir + "/textures/skybox/right.jpg",
    //         resourceDir + "/textures/skybox/left.jpg",
    //         resourceDir + "/textures/skybox/top.jpg",
    //         resourceDir + "/textures/skybox/bottom.jpg",
    //         resourceDir + "/textures/skybox/front.jpg",
    //         resourceDir + "/textures/skybox/back.jpg"
    //     },
    //     false
    // });
    // skybox_obj.before_action_ = [&r](ShaderProgram &shader, RenderPass pass) {
    //     if (pass != RenderPass::Draw || !r.currentRenderCamera()) {
    //         return;
    //     }
    //     shader.setMat4(
    //         "no_translate_view",
    //         glm::mat4(glm::mat3(r.currentRenderCamera()->viewMatrix()))
    //     );
    // };
    // r.objects().push_back(std::move(skybox_obj));

    // auto glasses = createGlasses(r, resourceDir);
    // for (auto &g : glasses) {
    //     r.transparentObjects().push_back(std::move(g));
    // }
}

void rotatePointLightsAroundAxis(Renderer &r, float angle_rad,
                                 const glm::vec3 &axis = glm::vec3{ 1.0f, 1.0f, 0.0f },
                                 const glm::vec3 &pivot = glm::vec3{ 0.0f },
                                 const glm::vec3 &orbit_offset = glm::vec3{ 0.0f, 0.0f, 5.0f })
{
    auto &lights = r.lights();
    const glm::vec3 n = glm::normalize(axis);

    glm::vec3 helper = (std::abs(n.y) < 0.99f) ? glm::vec3{ 0.0f, 1.0f, 0.0f }
                                               : glm::vec3{ 1.0f, 0.0f, 0.0f };
    const glm::vec3 u = glm::normalize(glm::cross(helper, n));
    const glm::vec3 v = glm::cross(n, u);
    const glm::vec3 local_offset = u * orbit_offset.x + n * orbit_offset.y + v * orbit_offset.z;

    const glm::mat4 rot = glm::rotate(glm::mat4{ 1.0f }, angle_rad, n);
    const glm::vec3 rotated_offset = glm::vec3(rot * glm::vec4{ local_offset, 0.0f });

    for (int i = 0; i < lights.counts.y; ++i) {
        lights.point[i].pos_ = glm::vec4{ pivot + rotated_offset, 1.0f };
    }
    r.winData().lights_UBO.setSubData(0, sizeof(LightData), &lights);
    syncPointLightMarkers(lights);
}

// ---------------------------------------------------------------------------

int main(int argc, char *argv[])
try {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <resources dir>" << std::endl;
        return -1;
    }
    const std::string resourceDir{ argv[1] };

    Renderer renderer;
    renderer.loadShaders(resourceDir);
    createScene(renderer, resourceDir);
    renderer.initShadowResources();
    renderer.initMirror();

    renderer.beginFrameTiming();
    float point_light_angle = 0.0f;
    while (!renderer.shouldClose()) {
        point_light_angle += renderer.winData().delta_time.count() * 0.6f;
        rotatePointLightsAroundAxis(renderer, point_light_angle);
        renderer.renderFrame();
        renderer.endFrame();
        renderer.processInput();
    }
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return -1;
}
