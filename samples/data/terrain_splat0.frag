#version 120
uniform vec4 s_global_ambient;
uniform vec4 s_material_ambient;
uniform vec4 s_material_diffuse;
uniform sampler2D textures[5];

varying vec2 frag_texcoord0;
varying vec4 frag_diffuse;

void main() {
    vec4 blend = texture2D(textures[4], frag_texcoord0.st);
    vec4 sand = texture2D(textures[0], frag_texcoord0.st) * vec4(blend.r);
    vec4 grass = texture2D(textures[1], frag_texcoord0.st) * vec4(blend.g);
    vec4 rock = texture2D(textures[2], frag_texcoord0.st) * vec4(blend.b);
    vec4 snow = texture2D(textures[3], frag_texcoord0.st) * vec4(blend.a);
    gl_FragColor = (s_global_ambient + s_material_ambient) * (sand + grass + rock + snow) * s_material_diffuse;
}
