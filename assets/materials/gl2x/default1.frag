#version {0}

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 s_light_color;
uniform float s_light_intensity;
uniform float s_light_range;
uniform vec4 s_light_position;

uniform sampler2D s_base_color_map;
uniform vec4 s_global_ambient;
uniform vec4 s_material_base_color;
uniform vec4 s_material_specular_color;
uniform float s_material_specular;
uniform mat4 s_base_color_map_matrix;

uniform int s_alpha_func;
uniform float s_alpha_threshold;

varying vec2 frag_texcoord0;
varying vec3 frag_position;
varying vec3 frag_normal;

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
    vec3 normal = normalize(frag_normal);

    vec3 light_direction = normalize(s_light_position.xyz - frag_position);
    float distance = length(s_light_position.xyz - frag_position);
    float attenuation = clamp(1.0 - (distance / s_light_range), 0.0, 1.0);

    vec4 base_color = texture2D(s_base_color_map, frag_texcoord0);
    base_color *= s_material_base_color;

    vec4 ambient = s_global_ambient * base_color;

    float diff = max(dot(normal, light_direction), 0.0);
    vec4 diffuse = s_light_color * s_light_intensity * diff * base_color * attenuation;

    vec3 view_direction = normalize(-frag_position);
    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), s_material_specular);
    vec4 specular = s_material_specular_color * s_light_color * s_light_intensity * spec * attenuation;

    vec4 final_color = ambient + diffuse + specular;
    alpha_test(final_color);
    gl_FragColor = final_color;
}

