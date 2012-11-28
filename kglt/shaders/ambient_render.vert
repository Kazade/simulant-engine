#version 120

attribute vec3 vertex_position;
attribute vec2 vertex_texcoord_1;

uniform mat4 modelview_projection_matrix;
uniform mat4 texture_matrix[1];

varying vec2 fragment_texcoord_1;

void main() {
    vec4 vertex = (modelview_projection_matrix * vec4(vertex_position, 1.0));
    fragment_texcoord_1 = (texture_matrix[0] * vec4(vertex_texcoord_1, 0, 1)).st;
    gl_Position = vertex;
}

