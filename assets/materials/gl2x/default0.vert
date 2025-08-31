#version {0}

#ifdef GL_ES
precision mediump float;
#endif

attribute vec3 s_position;
attribute vec2 s_texcoord0;
attribute vec3 s_normal;
attribute vec4 s_color;

uniform mat4 s_modelview_projection;
uniform mat4 s_base_color_map_matrix;

varying vec2 frag_texcoord0;
varying vec4 frag_color;
varying vec4 frag_position;
varying vec3 frag_normal;

void main() {
    frag_texcoord0 = (s_base_color_map_matrix * vec4(s_texcoord0, 0, 1)).st;
    frag_color = s_color;
    frag_normal = s_normal;
    frag_position = vec4(s_position, 1.0);
    gl_Position = (s_modelview_projection * vec4(s_position, 1.0));
}
