#version 120

varying vec2 fragment_texcoord[8];

uniform sampler2D textures[8];

uniform vec4 global_ambient;

void main() {
    gl_FragColor = vec4(0, 0, 0, 0);
    
    for(int i = 0; i < 8; ++i) {
        gl_FragColor += texture2D(textures[i], fragment_texcoord[i].st);
    }
    
    gl_FragColor = gl_FragColor * global_ambient;
}

