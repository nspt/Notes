#pragma once

#include "Material.h"
#include "Mesh.h"
#include <memory>

class RenderObject {
public:
    std::shared_ptr<Material> material_;
    std::shared_ptr<Mesh> mesh_;
};