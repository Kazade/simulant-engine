#version {0}

#ifdef GL_ES
precision mediump float;
#endif

attribute vec3 s_position;
attribute vec4 s_color;

uniform vec4 s_material_base_color;
uniform mat4 s_modelview_projection;
uniform float s_point_size;

varying vec4 diffuse;

void main() {
    diffuse = s_color * s_material_base_color;
    gl_Position = (s_modelview_projection * vec4(s_position, 1.0));
    gl_PointSize = s_point_size;
}
