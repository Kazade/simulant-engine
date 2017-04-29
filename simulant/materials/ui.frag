#version 120

varying vec2 frag_texcoord;
varying vec4 frag_colour;

uniform sampler2D texture_unit;

void main() {
    gl_FragColor = texture2D(texture_unit, frag_texcoord) * frag_colour;
}
