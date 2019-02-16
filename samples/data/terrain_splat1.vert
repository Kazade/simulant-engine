#version 120
attribute vec3 s_position;
attribute vec4 s_diffuse;
attribute vec3 s_normal;
attribute vec2 s_texcoord0;
attribute vec2 s_texcoord1;

uniform mat4 s_modelview;
uniform mat4 s_modelview_projection;
uniform mat4 s_view;
uniform mat3 s_inverse_transpose_modelview;
uniform vec4 s_light_position;

varying vec4 vertex_position_eye;
varying vec4 vertex_normal_eye;
varying vec4 light_position_eye;
varying vec4 frag_diffuse;

void main() {
    vertex_normal_eye = vec4(normalize(s_inverse_transpose_modelview * s_normal), 0); //Calculate the normal
    vertex_position_eye = (s_modelview * vec4(s_position, 1.0));
    light_position_eye = (s_view * s_light_position);
    frag_diffuse = s_diffuse;

    gl_Position = (s_modelview_projection * vec4(s_position, 1.0));
}
