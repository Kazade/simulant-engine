#version {0}

#ifdef GL_ES
precision highp float;
#endif

uniform vec4 s_global_ambient;
uniform vec4 s_material_ambient;
uniform vec4 s_material_diffuse;
uniform sampler2D s_diffuse_map;
uniform sampler2D s_light_map;
uniform bool s_lighting_enabled;

varying vec2 frag_texcoord0;
varying vec2 frag_texcoord1;
varying vec4 frag_diffuse;

void main() {
    vec4 ambient = (s_lighting_enabled) ? s_material_ambient * s_global_ambient : vec4(1);
    vec4 diffuse = (s_lighting_enabled) ? s_material_diffuse : frag_diffuse;
    
    vec4 t1 = texture2D(s_diffuse_map, frag_texcoord0.st);

    /* If lightmap is unbound, the default texture is white */
    vec4 t2 = texture2D(s_light_map, frag_texcoord1.st);

    gl_FragColor = ambient * diffuse * t1 * t2;
}
