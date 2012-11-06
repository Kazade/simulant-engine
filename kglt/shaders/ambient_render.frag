#version 120

varying vec2 fragment_texcoord_1;

uniform sampler2D texture_1;

uniform vec4 global_ambient;

void main() {
    gl_FragColor = texture2D(texture_1, fragment_texcoord_1.st) * global_ambient;
}

