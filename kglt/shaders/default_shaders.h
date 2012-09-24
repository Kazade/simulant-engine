#ifndef DEFAULT_SHADERS_H
#define DEFAULT_SHADERS_H

#include <string>

const std::string ambient_render_vert = R"(
    #version 120

attribute vec3 vertex_position;
attribute vec2 vertex_texcoord_1;
attribute vec4 vertex_diffuse;

uniform mat4 modelview_projection_matrix;

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

void main() {
    vec4 vertex = (modelview_projection_matrix * vec4(vertex_position, 1.0));
    fragment_texcoord_1 = vertex_texcoord_1;
    fragment_diffuse = vertex_diffuse;
    gl_Position = vertex;
}


)";

const std::string ambient_render_frag = R"(
    #version 120

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

uniform sampler2D texture_1;

uniform vec4 global_ambient;

void main() {
    gl_FragColor = texture2D(texture_1, fragment_texcoord_1.st) * colour * global_ambient;
}


)";

const std::string phong_lighting_vert = R"(
    #version 120

attribute vec3 vertex_position;
attribute vec2 vertex_texcoord_1;
attribute vec4 vertex_diffuse;
attribute vec3 vertex_normal;

uniform mat4 modelview_projection_matrix;

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

uniform vec4 light_position;

varying vec3 light_direction;
varying vec3 fragment_normal;
varying vec3 eye_vec;
varying float dist;

void main() {
    vec4 vertex = (modelview_projection_matrix * vec4(vertex_position, 1.0));

    light_direction = light_position.xyz - vertex.xyz;
    dist = length(light_direction);

    fragment_normal = vertex_normal;
    eye_vec = -vertex.xyz;
    fragment_texcoord_1 = vertex_texcoord_1;
    fragment_diffuse = vertex_diffuse;

    gl_Position = vertex;
}


)";

const std::string phong_lighting_frag = R"(
    #version 120

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

uniform sampler2D texture_1;

uniform vec4 global_ambient;
uniform vec4 light_ambient;
uniform vec4 light_diffuse;
uniform vec4 light_specular;

uniform float light_constant_attenuation;
uniform float light_linear_attenuation;
uniform float light_quadratic_attenuation;

varying vec3 light_direction;
varying vec3 fragment_normal;
varying vec3 eye_vec;
varying float dist;

void main() {
    vec4 material_ambient = vec4(0.1, 0.1, 0.1, 1.0);
    vec4 material_diffuse = vec4(1.0);
    vec4 material_specular = vec4(0.1);
    float material_shininess = 0.1;

    vec3 N = normalize(fragment_normal);
    vec3 L = normalize(light_direction);

    float lt = dot(N, L);

    float attenuation = 1.0 / (light_constant_attenuation +
                               light_linear_attenuation * dist +
                               light_quadratic_attenuation * dist * dist);

    vec4 colour = global_ambient + (light_ambient * material_ambient * attenuation);
    if(lt > 0.0) {
        colour += light_diffuse * material_diffuse * lt * attenuation;
        vec3 E = normalize(eye_vec);
        vec3 R = reflect(-L, N);
        float specular = pow(max(dot(R, E), 0.0), material_shininess);
        colour += (light_specular * material_specular * specular) * attenuation;
    }

    gl_FragColor = /*texture2D(texture_1, fragment_texcoord_1.st) * */ colour;
}


)";
#endif
