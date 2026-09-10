#version 330 core

in vec2 v_tex_coord;
in vec3 v_frag_pos;

#include "common/material.glsl"

uniform vec3 light_pos;
uniform float far_plane;

void main()
{
    if (!material.pure_color && material.diffuse_exist) {
        vec4 diffuse = texture(material.diffuse_texture, v_tex_coord);
        if (diffuse.a < 0.01)
            discard;
    }

    float distance = length(v_frag_pos - light_pos);
    distance = distance / far_plane;
    gl_FragDepth = distance;
}
