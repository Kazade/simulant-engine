#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D s_base_color_map;
varying vec2 frag_texcoord0;
varying vec4 frag_diffuse;

void main() {
    gl_FragColor = texture2D(s_base_color_map, frag_texcoord0.st) * frag_diffuse;
}
