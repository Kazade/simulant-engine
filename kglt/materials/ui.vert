#version 120

attribute vec2 position;
attribute vec2 tex_coord;
attribute vec4 colour;

varying vec4 frag_colour;
varying vec2 frag_texcoord;

uniform mat4 modelview_projection;

void main() {
    gl_Position = modelview_projection * vec4(position, 0.0, 1.0);
    frag_texcoord = tex_coord;
    frag_colour = colour;
}
