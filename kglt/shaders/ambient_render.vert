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

