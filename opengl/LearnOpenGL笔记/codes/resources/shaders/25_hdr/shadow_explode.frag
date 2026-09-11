#version 330 core

in vec2 v_uv_coord;

#include "common/material.glsl"

void main()
{
    if (!material.pure_color && material.diffuse_exist) {
        vec4 diffuse = texture(material.diffuse_texture, v_uv_coord);
        if (diffuse.a < 0.01)
            discard;
    }
}
