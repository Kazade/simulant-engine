#version {0}

#ifdef GL_ES
precision highp float;
#endif

attribute vec3 s_position;
attribute vec2 s_texcoord0;
attribute vec2 s_texcoord1;
attribute vec4 s_diffuse;

uniform mat4 s_modelview_projection;
uniform float s_point_size;
uniform mat4 s_diffuse_map_matrix;
uniform mat4 s_light_map_matrix;

varying vec2 frag_texcoord0;
varying vec2 frag_texcoord1;
varying vec4 frag_diffuse;

void main() {
    frag_texcoord0 = (s_diffuse_map_matrix * vec4(s_texcoord0, 0, 1)).st;
    frag_texcoord1 = (s_light_map_matrix * vec4(s_texcoord1, 0, 1)).st;
    frag_diffuse = s_diffuse;
    gl_Position = (s_modelview_projection * vec4(s_position, 1.0));
    gl_PointSize = s_point_size;
}
