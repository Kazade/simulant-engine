#version 120
attribute vec3 s_position;
attribute vec4 s_diffuse;
attribute vec2 s_texcoord0;

uniform mat4 s_modelview_projection;
uniform float s_point_size;

varying vec2 frag_texcoord0;
varying vec4 frag_diffuse;

void main() {
    frag_texcoord0 = s_texcoord0;
    frag_diffuse = s_diffuse;
    gl_Position = (s_modelview_projection * vec4(s_position, 1.0));
    gl_PointSize = s_point_size;
}
