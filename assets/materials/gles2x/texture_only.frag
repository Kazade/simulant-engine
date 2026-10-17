#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D s_base_color_map;
uniform int s_alpha_func;
uniform float s_alpha_threshold;

varying vec2 frag_texcoord0;
varying vec4 frag_diffuse;

void alpha_test(vec4 c) {
    if(s_alpha_func == 1) {        // LESS
        if(c.a >= s_alpha_threshold) discard;
    } else if(s_alpha_func == 2) { // LEQUAL
        if(c.a > s_alpha_threshold) discard;
    } else if(s_alpha_func == 3) { // EQUAL
        if(c.a != s_alpha_threshold) discard;
    } else if(s_alpha_func == 4) { // GEQUAL
        if(c.a < s_alpha_threshold) discard;
    } else if(s_alpha_func == 5) { // GREATER
        if(c.a <= s_alpha_threshold) discard;
    }
}

void main() {
    vec4 color = texture2D(s_base_color_map, frag_texcoord0.st) * frag_diffuse;
    alpha_test(color);
    gl_FragColor = color;
}
