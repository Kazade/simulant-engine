#ifndef DEFAULT_SHADERS_H
#define DEFAULT_SHADERS_H

#include <string>

const std::string ambient_render_vert = R"(
#version 120

attribute vec3 vertex_position;

attribute vec2 vertex_texcoord_0;
attribute vec2 vertex_texcoord_1;
attribute vec2 vertex_texcoord_2;
attribute vec2 vertex_texcoord_3;
attribute vec2 vertex_texcoord_4;
attribute vec2 vertex_texcoord_5;
attribute vec2 vertex_texcoord_6;
attribute vec2 vertex_texcoord_7;

uniform mat4 modelview_projection_matrix;
uniform mat4 texture_matrix[8];
uniform int active_texture_count;

varying vec2 fragment_texcoord[8];

void main() {
    vec2 vertex_texcoords[8];
    
    vertex_texcoords[0] = vertex_texcoord_0;
    vertex_texcoords[1] = vertex_texcoord_1;
    vertex_texcoords[2] = vertex_texcoord_2;
    vertex_texcoords[3] = vertex_texcoord_3;
    vertex_texcoords[4] = vertex_texcoord_4;
    vertex_texcoords[5] = vertex_texcoord_5;
    vertex_texcoords[6] = vertex_texcoord_6;
    vertex_texcoords[7] = vertex_texcoord_7;

    vec4 vertex = (modelview_projection_matrix * vec4(vertex_position, 1.0));
    for(int i = 0; i < active_texture_count; ++i) {
        fragment_texcoord[i] = (texture_matrix[i] * vec4(vertex_texcoords[i], 0, 1)).st;
    }
    gl_Position = vertex;
}


)";

const std::string phong_lighting_vert = R"(
#version 120

attribute vec3 vertex_position;
attribute vec2 vertex_texcoord_1;
attribute vec4 vertex_diffuse;
attribute vec3 vertex_normal;

uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

uniform vec4 light_position;

varying vec3 light_direction;
varying vec3 fragment_normal;
varying vec3 eye_vec;
varying float dist;

void main() {
    vec4 vertex = (modelview_matrix * vec4(vertex_position, 1.0));

    light_direction = light_position.xyz - vertex.xyz;
    dist = length(light_direction);

    vec4 final = (modelview_projection_matrix * vec4(vertex_position, 1.0));
    eye_vec = -final.xyz;
    
    fragment_texcoord_1 = vertex_texcoord_1;
    fragment_diffuse = vertex_diffuse;
    fragment_normal = vertex_normal;
        
    gl_Position = final;
}


)";

const std::string ambient_render_frag = R"(
#version 120

varying vec2 fragment_texcoord[8];

uniform sampler2D textures[8];
uniform vec4 global_ambient;
uniform int active_texture_count;

void main() {
    gl_FragColor = texture2D(textures[0], fragment_texcoord[0].st);
    
    for(int i = 1; i < 8; ++i) {
        if(i >= active_texture_count) break;
        
        vec4 t1 = gl_FragColor;
        vec4 t2 = texture2D(textures[i], fragment_texcoord[i].st);
        gl_FragColor = ((1.0 - t2.a) * t1) + (t2.a * t2);
    }
    
    gl_FragColor = gl_FragColor * global_ambient;
}


)";

const std::string phong_lighting_frag = R"(
#version 120

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

uniform sampler2D texture_1;

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

    vec4 colour = (light_ambient * material_ambient * attenuation);
    if(lt > 0.0) {
        colour += light_diffuse * material_diffuse * lt * attenuation;
        vec3 E = normalize(eye_vec);
        vec3 R = reflect(-L, N);
        float specular = pow(max(dot(R, E), 0.0), material_shininess);
        colour += (light_specular * material_specular * specular) * attenuation;
    }

    gl_FragColor = colour;
}


)";
#endif
