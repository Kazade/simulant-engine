#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 s_light_ambient;
uniform vec4 s_light_diffuse;
uniform vec4 s_light_specular;
uniform float s_light_constant_attenuation;
uniform float s_light_linear_attenuation;
uniform float s_light_quadratic_attenuation;

uniform sampler2D s_diffuse_map;
uniform vec4 s_global_ambient;
uniform vec4 s_material_ambient;
uniform vec4 s_material_diffuse;
uniform vec4 s_material_specular;
uniform float s_material_shininess;
uniform mat4 s_diffuse_map_matrix;

varying vec4 vertex_position_eye;
varying vec4 vertex_normal_eye;
varying vec4 light_position_eye;
varying vec2 frag_texcoord0;

vec4 ambient_lighting() {
    return s_material_ambient * s_light_ambient;
}

vec4 diffuse_lighting(vec4 N, vec4 L) {
    float term = clamp(dot(N, L), float(0.0), float(1.0));
    return s_material_diffuse * s_light_diffuse * term;
}

vec4 specular_lighting(vec4 N, vec4 L, vec4 V) {
    float term = 0.0;
    if(dot(N, L) > float(0.0)) {
        vec4 H = normalize(L + V);
        term = pow(dot(N, N), s_material_shininess);
    }

    return s_material_specular * s_light_specular * term;
}

void main() {
    vec4 light_dir = light_position_eye - (vertex_position_eye * vec4(light_position_eye.w));
    vec4 n_eye = normalize(vertex_normal_eye);
    vec4 s_eye = normalize(light_dir);
    vec4 v_eye = normalize(-vertex_position_eye);
    vec4 h_eye = normalize(v_eye + s_eye);

    vec4 amb = ambient_lighting();
    vec4 diff = diffuse_lighting(n_eye, s_eye);
    vec4 spec = specular_lighting(n_eye, s_eye, v_eye);

    vec4 map_diffuse = texture2D(s_diffuse_map, frag_texcoord0.st);

    float attenuation = 1.0;

    if(light_position_eye.w > 0.0) {
        float d = length(light_dir);
        attenuation = 1.0 / (
            s_light_constant_attenuation +
            s_light_linear_attenuation * d +
            s_light_quadratic_attenuation * d * d
        );
    }

    gl_FragColor = map_diffuse * (amb + diff + spec) * attenuation;
}

