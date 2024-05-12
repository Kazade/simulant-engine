#version {0}

#ifdef GL_ES
precision mediump float;
#endif

varying vec4 diffuse;
void main() {
    gl_FragColor = diffuse;
}
