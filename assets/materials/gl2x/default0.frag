#version {0}

#ifdef GL_ES
#extension GL_OES_standard_derivatives : enable
precision mediump float;
#endif

varying vec2 frag_texcoord0;
varying vec3 frag_normal;
varying vec3 frag_position;
varying vec4 frag_color;

uniform int s_alpha_func;
uniform float s_alpha_threshold;

uniform sampler2D s_base_color_map;
uniform sampler2D s_normal_map;
uniform vec4 s_material_base_color;
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

// Derivative-based TBN — no tangent vertex attribute required.
// Reconstructs tangent and bitangent from screen-space derivatives of the
// eye-space position and texture coordinates.
mat3 cotangent_frame(vec3 N, vec3 pos, vec2 uv) {
    vec3 dp1  = dFdx(pos);
    vec3 dp2  = dFdy(pos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

void main() {
    vec4 base_color_tex = texture2D(s_base_color_map, frag_texcoord0);
    vec3 base_color = mix(s_material_base_color.rgb, base_color_tex.rgb, base_color_tex.a);
    float alpha = s_material_base_color.a * base_color_tex.a;

    base_color *= frag_color.rgb;

    float metallic  = s_material_metallic;
    float roughness = s_material_roughness;

    // METALLIC_ROUGHNESS_MAP_ENABLED = 8 (bit 3)
    if (s_textures_enabled >= 8) {
        vec4 mr = texture2D(s_metallic_roughness_map, frag_texcoord0);
        metallic  = mr.r;
        roughness = mr.g;
    }

    vec3 N = normalize(frag_normal);
    vec3 V = normalize(-frag_position);

    // NORMAL_MAP_ENABLED = 4 (bit 2): extract via modular arithmetic (no bitwise ops in GLSL 1.20/ES 1.00)
    if (mod(float(s_textures_enabled), 8.0) >= 4.0) {
        vec3 nm = texture2D(s_normal_map, frag_texcoord0).rgb * 2.0 - 1.0;
        mat3 TBN = cotangent_frame(N, frag_position, frag_texcoord0);
        N = normalize(TBN * nm);
    }

    // NdotV is view-dependent and constant across lights
    float NdotV = max(dot(N, V), 0.0001);

    vec3 color;

    if (s_lighting_enabled == 0) {
        color = base_color * s_global_ambient.rgb;
    } else {
        color = base_color * s_global_ambient.rgb;

        vec3 F0 = mix(vec3(0.04), base_color, metallic);

        // Roughness remapped to alpha for GGX
        float alpha    = roughness * roughness;
        float alphaSq  = alpha * alpha;
        // Schlick-GGX k for Smith geometry term
        float k = alpha / 2.0;

        for (int i = 0; i < s_light_count; i++) {
            vec3 L;
            float att;

            if (s_light_position[i].w < 0.5) {
                // Directional light: the engine stores the negated direction in
                // the light's position (see Light::set_direction), so xyz is
                // already the toward-light vector in world space.  Transform it
                // as a direction (w=0) so translation is ignored.
                L   = normalize(vec3(s_view * vec4(s_light_position[i].xyz, 0.0)));
                att = 1.0;
            } else {
                // Point light: xyz is world-space position.
                vec3 light_pos_eye = vec3(s_view * vec4(s_light_position[i].xyz, 1.0));
                vec3 to_light = light_pos_eye - frag_position;
                float dist = length(to_light);
                L = to_light / max(dist, 0.001);

                float range = s_light_range[i];
                att = clamp(1.0 - (dist / max(range, 0.001)), 0.0, 1.0);
            }

            float NdotL = max(dot(N, L), 0.0);
            vec3  H     = normalize(L + V);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);

            // GGX normal distribution
            float d = NdotH * NdotH * (alphaSq - 1.0) + 1.0;
            float D = alphaSq / max(PI * d * d, 0.0001);

            // Smith Schlick-GGX geometry (view + light)
            float GV = NdotV / (NdotV * (1.0 - k) + k);
            float GL = NdotL / max(NdotL * (1.0 - k) + k, 0.0001);
            float G  = GV * GL;

            // Fresnel — use half-vector/view angle for physical accuracy
            vec3 F = fresnelSchlick(HdotV, F0);

            // Cook-Torrance specular
            vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);

            // Energy-conserving Lambertian diffuse
            vec3 kD      = (vec3(1.0) - F) * (1.0 - metallic);
            vec3 diffuse = kD * base_color;

            color += (diffuse + spec) * NdotL * s_light_color[i].xyz * s_light_intensity[i] * att;
        }
    }

    color = clamp(color, 0.0, 1.0);
    vec4 final_color = vec4(color, alpha);
    alpha_test(final_color);
    gl_FragColor = final_color;
}
