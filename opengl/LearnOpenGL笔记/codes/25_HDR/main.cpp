#include <cmath>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Buffers/InstanceBuffer.h"
#include "GLFWWin.h"
#include "Model/Material.h"
#include "Model/Model.h"
#include "Model/RenderObject.h"
#include "Renderer/Renderer.h"
#include "Scene/Helper.h"
#include "Scene/Scene.h"

namespace {

constexpr int window_width = 1280;
constexpr int window_height = 720;
constexpr float kPointLightMarkerScale = 0.1f;
constexpr float kSpotLightMarkerScale = 0.3f;

LightData createLightData()
{
    LightData lights;
    lights.counts.x = 1;
    lights.directional[0].direction_ = glm::normalize(glm::vec4{ 0.0f, -1.0f, 0.0f, 0.0f });
    lights.directional[0].ambient_ = glm::vec4{ 0.05f };
    lights.directional[0].diffuse_ = glm::vec4{ 0.8f };
    lights.directional[0].specular_ = glm::vec4{ 1.0f };

    // 高亮白光在容器远端；其余彩色点光分布在它与原点之间，xy∈[-1,1]
    lights.point[0].pos_ = glm::vec4{ 0.0f, 0.0f, -9.8f, 1.0f };
    lights.point[0].ambient_ = glm::vec4{ 0.05f };
    lights.point[0].diffuse_ = glm::vec4{ 100.0f };
    lights.point[0].specular_ = glm::vec4{ 100.0f };
    lights.point[0].attenuation = glm::vec4{ 1.0f, 0.09f, 0.032f, 0.0f };

    const glm::vec4 attenuation{ 1.0f, 0.09f, 0.032f, 0.0f };
    const struct {
        glm::vec3 pos;
        glm::vec3 color;
        float intensity;
    } colored_points[] = {
        { {  0.6f, -0.5f,  -2.0f }, { 1.0f, 0.2f, 0.1f }, 10.0f },  // 红
        { { -0.7f,  0.7f,  -4.5f }, { 0.1f, 1.0f, 0.2f }, 10.0f },  // 绿
        { {  0.4f,  0.4f,  -7.0f }, { 0.2f, 0.4f, 1.0f }, 10.0f },  // 蓝
        { { -0.5f, -0.6f,  -9.5f }, { 1.0f, 0.85f, 0.1f }, 10.0f }, // 黄
    };
    auto colored_points_count = sizeof(colored_points) / sizeof(colored_points[0]);
    lights.counts.y = colored_points_count + 1;
    for (int i = 0; i < colored_points_count; ++i) {
        const auto &p = colored_points[i];
        lights.point[i + 1].pos_ = glm::vec4{ p.pos, 1.0f };
        lights.point[i + 1].ambient_ = glm::vec4{ 0.0f };
        lights.point[i + 1].diffuse_ = glm::vec4{ p.color * p.intensity, 1.0f };
        lights.point[i + 1].specular_ = glm::vec4{ p.color * p.intensity, 1.0f };
        lights.point[i + 1].attenuation = attenuation;
    }
    return lights;
}

InstanceBuffer::InstanceData spotMarkerInstance(const SpotLight &spot)
{
    InstanceBuffer::InstanceData instance;
    instance.translation_ = glm::vec3(spot.pos_);
    const glm::vec3 direction{ spot.direction_inner_ };
    const float yaw = std::atan2(direction.x, direction.z);
    const float horizontal = std::hypot(direction.x, direction.z);
    const float pitch = std::atan2(-direction.y, horizontal);
    instance.rotation_ = glm::angleAxis(yaw, glm::vec3{ 0.0f, 1.0f, 0.0f })
        * glm::angleAxis(pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });
    instance.scale_ = glm::vec3{ kSpotLightMarkerScale };
    return instance;
}

std::vector<Model> createLightMarkers(Renderer &renderer, Scene scene, const std::string &resourceDir)
{
    std::vector<Model> models;
    const auto &lights = scene.data_->lights_;

    for (int i = 0; i < lights.counts.y; ++i) {
        Material material;
        material.pure_color_ = true;
        material.color_ = glm::vec3(lights.point[i].diffuse_);

        Model marker{ RenderObject{
            material, createCubeMesh(),
            renderer.shader("lit"), ShaderProgram{}, ShaderProgram{}
        } };
        marker.setInstances(InstanceBuffer::InstanceData{
            .translation_ = glm::vec3(lights.point[i].pos_),
            .scale_ = glm::vec3{ kPointLightMarkerScale }
        });
        marker.update_action_ = [scene, i](Model &self, auto, auto) {
            if (i >= scene.data_->lights_.counts.y) {
                return;
            }
            const auto &pl = scene.data_->lights_.point[i];
            self.setInstances(InstanceBuffer::InstanceData{
                .translation_ = glm::vec3(pl.pos_),
                .scale_ = glm::vec3{ kPointLightMarkerScale }
            });
            for (auto &o : self.objects_) {
                o.material_.color_ = glm::vec3(pl.diffuse_);
            }
        };
        models.push_back(std::move(marker));
    }

    if (lights.counts.z > 0) {
        Model flashlight{
            resourceDir + "/model/flash_light", "Flashlight.obj",
            renderer.shader("lit"), ShaderProgram{}, ShaderProgram{}, false
        };

        for (int i = 0; i < lights.counts.z; ++i) {
            Model marker = flashlight;
            marker.setInstances(spotMarkerInstance(lights.spot[i]));
            marker.update_action_ = [scene, i](Model &self, auto, auto) {
                if (i >= scene.data_->lights_.counts.z) {
                    return;
                }
                self.setInstances(spotMarkerInstance(scene.data_->lights_.spot[i]));
            };
            models.push_back(std::move(marker));
        }
    }

    return models;
}

Scene createScene(Renderer &renderer, const std::string &resourceDir)
{
    Scene scene;
    scene.data_->lights_ = createLightData();

    scene.data_->models_.push_back(createContainerCube(renderer, resourceDir));
    scene.data_->models_.back().setInstances(InstanceBuffer::InstanceData{
        .scale_ = glm::vec3{ 1.0f, 1.f, 10.0f }
    });

    auto markers = createLightMarkers(renderer, scene, resourceDir);
    scene.data_->models_.insert(
        scene.data_->models_.end(),
        std::make_move_iterator(markers.begin()),
        std::make_move_iterator(markers.end())
    );
    return scene;
}

void syncRendererFromWindow(Renderer &renderer, const WindowState &win)
{
    renderer.setCameraData(win.camera.cameraData());
    renderer.setViewport(0, 0, win.width, win.height);
    auto params = renderer.postProcParams();
    params.exposure = win.exposure;
    params.gamma = win.gamma;
    renderer.setPostProcParams(params);
}

Renderer createRenderer(const std::string &resourceDir, const WindowState &win)
{
    Renderer renderer;
    renderer.setShaders(loadShaders(resourceDir, "25_hdr"));
    renderer.setPostProcShader(renderer.shader("quad"));
    renderer.setBlurShader(renderer.shader("blur"));
    renderer.setCompositeShader(renderer.shader("composite"));
    syncRendererFromWindow(renderer, win);
    return renderer;
}

} // namespace

int main(int argc, char *argv[])
try {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <resources dir>" << std::endl;
        return -1;
    }
    const std::string resourceDir{ argv[1] };

    WindowState win;
    win.camera.moveTo(glm::vec3{ 0.0f, 0.0f, 4.0f });
    // 必须先于 Renderer 构造：Renderer 创建 GL 资源需要 context
    GLFWWin window{ window_width, window_height, "HDR", win };

    Renderer renderer = createRenderer(resourceDir, win);

    Scene scene = createScene(renderer, resourceDir);
    renderer.setScene(scene);

    while (!window.shouldClose()) {
        window.beginFrame(scene);

        syncRendererFromWindow(renderer, win);
        renderer.render();

        window.endFrame(scene);
    }
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return -1;
}
