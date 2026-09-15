#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <vector>
#include "Light.h"
#include "../Model/Model.h"
#include "../Model/Skybox.h"

class Scene {
public:
    struct Data {
        LightData lights_;
        std::vector<Model> models_;
        std::vector<Model> transparent_models_;
        std::optional<Skybox> skybox_;
        std::optional<std::chrono::steady_clock::time_point> start_tp_;
        std::optional<std::chrono::steady_clock::time_point> last_update_tp_;
    };
    std::shared_ptr<Data> data_{ std::make_shared<Data>() };
};
