#version 330 core

in vec3 frag_normal;
in vec3 frag_pos;

out vec4 frag_color;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * lightColor;

    vec3 norm = normalize(frag_normal);
    vec3 lightDir = normalize(lightPos - frag_pos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 viewDir = normalize(viewPos - frag_pos);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    frag_color = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}