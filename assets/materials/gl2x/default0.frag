#version {0}

#ifdef GL_ES
precision mediump float;
#endif

// Input attributes
varying vec2 frag_texcoord0; // Texture coordinates
varying vec3 frag_normal;    // Normal vector
varying vec4 frag_position;  // Fragment position
varying vec4 frag_color;    // Fragment color

// Uniforms
uniform int s_alpha_func;
uniform float s_alpha_threshold;

uniform sampler2D s_base_color_map; // Base color texture
uniform vec4 s_material_base_color;              // Base color
uniform vec4 s_material_specular_color;          // Specular color
uniform float s_material_specular;              // Specular intensity
uniform float s_material_metallic;       // Metallic map
uniform float s_material_roughness;      // Roughness map
uniform sampler2D s_metallic_roughness_map; // Combined metallic and roughness map
uniform int s_textures_enabled;

const float PI = 3.14159265359;

uniform vec4 s_light_position[8];
uniform vec4 s_light_color[8];
uniform float s_light_intensity[8]; 
uniform int s_light_count;
uniform vec4 s_global_ambient; // Global ambient light


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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // Sample the base color and metallic/roughness maps
    vec4 base_color_tex = texture2D(s_base_color_map, frag_texcoord0);
    vec3 base_color = mix(s_material_base_color.rgb, base_color_tex.rgb, base_color_tex.a);
    float alpha = s_material_base_color.a * base_color_tex.a; // Retain alpha

    float metallic = s_material_metallic;
    float roughness = s_material_roughness;

    // If using a combined metallic/roughness map
    if (s_textures_enabled >= 8) {
        vec4 metallic_roughness = texture2D(s_metallic_roughness_map, frag_texcoord0);
        metallic = metallic_roughness.r;
        roughness = metallic_roughness.g;
    }

    // Calculate the normal and view direction
    vec3 N = normalize(frag_normal);
    vec3 V = normalize(-frag_position.xyz); // Assuming camera is at the origin

    // Initialize color with global ambient contribution
    vec3 color = base_color * s_global_ambient.rgb;

    for (int i = 0; i < s_light_count; i++) {
        // Light position is in eye space, so we can use it directly
        vec3 L = normalize(s_light_position[i].xyz); // Light direction from the eye to the light

        // Calculate the distance from the fragment to the light
        float distance = length(s_light_position[i].xyz); // Distance from the eye to the light
        float att = clamp(s_light_intensity[i] / (distance * distance), 0.0, 1.0);

        // Calculate the reflection vector
        vec3 R = reflect(-L, N);
        
        // Calculate the Fresnel term
        vec3 F0 = mix(vec3(0.04), base_color, metallic);
        float cosTheta = max(dot(V, N), 0.0);
        vec3 F = fresnelSchlick(cosTheta, F0);
        
        // Calculate the specular term using GGX
        float alphaVal = roughness * roughness;
        float D = alphaVal / (PI * pow((cosTheta * cosTheta * (alphaVal - 1.0) + 1.0), 2.0));
        
        // Calculate the geometric attenuation
        float k = alphaVal / 2.0;
        float G = cosTheta / (cosTheta * (1.0 - k) + k);
        
        // Final color calculation
        vec3 specular = (D * G * F) / (4.0 * cosTheta * cosTheta);
        vec3 diffuse = max(dot(N, L), 0.0) * s_light_color[i].xyz * s_light_intensity[i] * att;

        // Accumulate color contributions
        color += diffuse + (s_material_specular * s_material_specular_color.rgb * specular * att);
    }

    // Clamp the final color to avoid overflow
    color = clamp(color, 0.0, 1.0);
    vec4 final_color = vec4(color, alpha);

    // Perform alpha testing
    alpha_test(final_color);
    gl_FragColor = final_color;
}
