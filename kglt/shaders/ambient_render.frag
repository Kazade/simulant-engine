#version 120

varying vec2 fragment_texcoord[8];

uniform sampler2D textures[8];
uniform vec4 global_ambient;
uniform int active_texture_count;

void main() {
    gl_FragColor = texture2D(textures[0], fragment_texcoord[0].st);
    
    for(int i = 1; i < 8; ++i) {
        if(i >= active_texture_count) break;
        
        vec4 t1 = gl_FragColor;
        vec4 t2 = texture2D(textures[i], fragment_texcoord[i].st);
        gl_FragColor = ((1.0 - t2.a) * t1) + (t2.a * t2);
    }
    
    gl_FragColor = gl_FragColor * global_ambient;
}

