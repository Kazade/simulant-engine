#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform int s_alpha_func;
uniform float s_alpha_threshold;

varying vec4 diffuse;

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
    alpha_test(diffuse);
    gl_FragColor = diffuse;
}
