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
    for(int i = 0; i < 8; ++i) {
        fragment_texcoord[i] = (texture_matrix[i] * vec4(vertex_texcoords[i], 0, 1)).st;
    }
    gl_Position = vertex;
}

