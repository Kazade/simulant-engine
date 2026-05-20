#version {0}

#ifdef GL_ES
precision mediump float;
#endif

varying vec2 frag_texcoord0;
varying vec3 frag_normal;
varying vec3 frag_position;
varying vec4 frag_color;

uniform int s_alpha_func;
uniform float s_alpha_threshold;

uniform sampler2D s_base_color_map;
uniform vec4 s_material_base_color;
uniform vec4 s_material_specular_color;
uniform float s_material_specular;
uniform float s_material_metallic;
uniform float s_material_roughness;
uniform sampler2D s_metallic_roughness_map;
uniform int s_textures_enabled;
uniform int s_lighting_enabled;

const float PI = 3.14159265359;

uniform vec4 s_light_position[8];
uniform vec4 s_light_color[8];
uniform float s_light_intensity[8];
uniform float s_light_range[8];
uniform int s_light_count;
uniform vec4 s_global_ambient;

uniform mat4 s_view;

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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main() {
    vec4 base_color_tex = texture2D(s_base_color_map, frag_texcoord0);
    vec3 base_color = mix(s_material_base_color.rgb, base_color_tex.rgb, base_color_tex.a);
    float alpha = s_material_base_color.a * base_color_tex.a;

    base_color *= frag_color.rgb;

    float metallic = s_material_metallic;
    float roughness = s_material_roughness;

    if (s_textures_enabled >= 8) {
        vec4 metallic_roughness = texture2D(s_metallic_roughness_map, frag_texcoord0);
        metallic = metallic_roughness.r;
        roughness = metallic_roughness.g;
    }

    vec3 color;

    if (s_lighting_enabled == 0) {
        color = base_color * s_global_ambient.rgb;
    } else {
        vec3 N = normalize(frag_normal);
        vec3 V = normalize(-frag_position);

        color = base_color * s_global_ambient.rgb;

        for (int i = 0; i < s_light_count; i++) {
            // Transform world-space light position to eye space
            vec3 light_pos_eye = vec3(s_view * vec4(s_light_position[i].xyz, 1.0));
            vec3 to_light = light_pos_eye - frag_position;
            float distance = length(to_light);
            vec3 L = to_light / max(distance, 0.001);

            float range = s_light_range[i];
            float att = clamp(1.0 - (distance / max(range, 0.001)), 0.0, 1.0);

            vec3 F0 = mix(vec3(0.04), base_color, metallic);
            float NdotV = max(dot(N, V), 0.0);
            vec3 F = fresnelSchlick(NdotV, F0);

            float alphaVal = roughness * roughness;
            float NdotH = max(dot(N, normalize(L + V)), 0.0);
            float denom = PI * pow((NdotH * NdotH * (alphaVal - 1.0) + 1.0), 2.0);
            float D = alphaVal / max(denom, 0.0001);

            float k = alphaVal / 2.0;
            float G = NdotV / max(NdotV * (1.0 - k) + k, 0.0001);

            vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotV, 0.0001);
            float NdotL = max(dot(N, L), 0.0);
            vec3 diffuse = NdotL * s_light_color[i].xyz * s_light_intensity[i] * att;

            color += diffuse + (s_material_specular * s_material_specular_color.rgb * specular * att);
        }
    }

    color = clamp(color, 0.0, 1.0);
    vec4 final_color = vec4(color, alpha);
    alpha_test(final_color);
    gl_FragColor = final_color;
}
