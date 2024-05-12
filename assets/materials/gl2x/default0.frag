#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 s_global_ambient;
uniform vec4 s_material_ambient;
uniform vec4 s_material_diffuse;
uniform sampler2D s_diffuse_map;
uniform sampler2D s_light_map;
uniform bool s_lighting_enabled;

uniform int s_alpha_func;
uniform float s_alpha_threshold;

varying vec2 frag_texcoord0;
varying vec2 frag_texcoord1;
varying vec4 frag_diffuse;


void alpha_test(vec4 c) {
    if(s_alpha_func == 1) { // LESS
        if(c.a >= s_alpha_threshold) {
            discard;
        }
    } else if(s_alpha_func == 2)  { // LEQUAL            
        if(c.a > s_alpha_threshold) {
            discard;
        }
    } else if(s_alpha_func == 3) { // EQUAL
        if(c.a != s_alpha_threshold) {
            discard;
        }
    } else if(s_alpha_func == 4) { // GEQUAL
        if(c.a < s_alpha_threshold) {
            discard;
        }
    } else if(s_alpha_func == 5) { // GREATER
        if(c.a <= s_alpha_threshold) {
            discard;
        }
    }
}

void main() {
    vec4 ambient = (s_lighting_enabled) ? s_material_ambient * s_global_ambient : vec4(1);
    vec4 diffuse = (s_lighting_enabled) ? s_material_diffuse : frag_diffuse;
    
    vec4 t1 = texture2D(s_diffuse_map, frag_texcoord0.st);

    /* If lightmap is unbound, the default texture is white */
    vec4 t2 = texture2D(s_light_map, frag_texcoord1.st);

    vec4 c = ambient * diffuse * t1 * t2;
    
    alpha_test(c);

    gl_FragColor = c;
}
