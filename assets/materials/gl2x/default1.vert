#version {0}

#ifdef GL_ES
precision mediump float;
#endif

attribute vec3 s_position;
attribute vec2 s_texcoord0;
attribute vec3 s_normal;

uniform mat4 s_model;
uniform mat4 s_modelview_projection;
uniform mat4 s_base_color_map_matrix;

varying vec2 frag_texcoord0;
varying vec3 frag_position;                       // Fragment position in world space
varying vec3 frag_normal;                         // Fragment normal in world space

void main() {
    frag_position = vec3(s_model * vec4(s_position, 1.0));
    frag_normal = normalize(mat3(s_model) * s_normal);
    frag_texcoord0 = (s_base_color_map_matrix * vec4(s_texcoord0, 0, 1)).st;

    gl_Position = (s_modelview_projection * vec4(s_position, 1.0));
}
