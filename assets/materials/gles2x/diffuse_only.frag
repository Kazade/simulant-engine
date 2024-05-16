#version {0}

#ifdef GL_ES
precision highp float;
#endif

varying vec4 diffuse;
void main() {
    gl_FragColor = diffuse;
}
