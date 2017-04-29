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

