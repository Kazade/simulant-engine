#pragma once

#include <cmath>

namespace smlt {

/* Per-light state for software per-vertex PBR lighting.
 * position[3] == 0.0f → directional light; == 1.0f → point light.
 * dir[] is pre-normalized toward-light direction (directional lights only). */
struct VertexLightState {
    bool  enabled   = false;
    float position[4] = {0, 0, 0, 0}; /* xyz = eye-space pos, w = type */
    float dir[3]      = {0, 0, -1};   /* pre-normalized L for directional */
    float color[3]    = {1, 1, 1};
    float intensity   = 1.0f;
    float range       = 100.0f;
};

/* Compute a PBR-approximate per-vertex colour.
 *
 * All vectors are in eye/view space.
 *   N          – normalised vertex normal (3 floats)
 *   pos        – vertex position          (3 floats)
 *   base_color – material base colour     (RGBA, 4 floats, [0,1])
 *   metallic   – metallic factor          [0,1]
 *   roughness  – roughness factor         [0,1]
 *   ambient    – global ambient colour    (RGB,  3 floats, [0,1])
 *   lights     – light state array
 *   light_count
 *   out        – output RGBA              (4 floats, [0,1])
 *
 * Matches the Cook-Torrance + GGX + Smith model used in the GL2X fragment
 * shader (default0.frag).  Sacrifices per-fragment accuracy for speed –
 * specular highlights may appear at incorrect vertex positions on coarse
 * meshes but the energy balance is correct. */
inline void compute_pbr_vertex_color(
    const float* N,
    const float* pos,
    const float* base_color,
    float metallic,
    float roughness,
    const float* ambient,
    const VertexLightState* lights,
    int light_count,
    float* out)
{
    /* F0: dielectric base reflectance blended toward base_color for metals */
    const float F0_r = 0.04f + (base_color[0] - 0.04f) * metallic;
    const float F0_g = 0.04f + (base_color[1] - 0.04f) * metallic;
    const float F0_b = 0.04f + (base_color[2] - 0.04f) * metallic;

    /* Ambient */
    float total_r = base_color[0] * ambient[0];
    float total_g = base_color[1] * ambient[1];
    float total_b = base_color[2] * ambient[2];

    /* View direction – eye is at origin in view space */
    float Vx = -pos[0], Vy = -pos[1], Vz = -pos[2];
    {
        float v_len = sqrtf(Vx*Vx + Vy*Vy + Vz*Vz);
        if(v_len > 1e-8f) { float inv = 1.0f / v_len; Vx *= inv; Vy *= inv; Vz *= inv; }
    }
    float NdotV = N[0]*Vx + N[1]*Vy + N[2]*Vz;
    if(NdotV < 0.0001f) NdotV = 0.0001f;

    /* GGX roughness terms – constant across all lights for this vertex */
    const float alpha   = roughness * roughness;
    const float alphaSq = alpha * alpha;
    const float k       = alpha * 0.5f;
    const float one_minus_metallic = 1.0f - metallic;

    for(int i = 0; i < light_count; i++) {
        const VertexLightState& light = lights[i];
        if(!light.enabled) continue;

        float Lx, Ly, Lz;
        float att = 1.0f;

        if(light.position[3] < 0.5f) {
            /* Directional – use pre-normalised direction */
            Lx = light.dir[0]; Ly = light.dir[1]; Lz = light.dir[2];
        } else {
            /* Point light */
            Lx = light.position[0] - pos[0];
            Ly = light.position[1] - pos[1];
            Lz = light.position[2] - pos[2];
            float dist_sq = Lx*Lx + Ly*Ly + Lz*Lz;
            float dist    = sqrtf(dist_sq);
            if(dist > 1e-8f) { float inv = 1.0f / dist; Lx *= inv; Ly *= inv; Lz *= inv; }
            att = 1.0f - dist / (light.range + 1e-8f);
            if(att < 0.0f) att = 0.0f;
        }

        float NdotL = N[0]*Lx + N[1]*Ly + N[2]*Lz;
        if(NdotL <= 0.0f) continue;

        /* Half-vector */
        float Hx = Lx + Vx, Hy = Ly + Vy, Hz = Lz + Vz;
        {
            float h_len = sqrtf(Hx*Hx + Hy*Hy + Hz*Hz);
            if(h_len > 1e-8f) { float inv = 1.0f / h_len; Hx *= inv; Hy *= inv; Hz *= inv; }
        }
        float NdotH = N[0]*Hx + N[1]*Hy + N[2]*Hz;
        if(NdotH < 0.0f) NdotH = 0.0f;
        float HdotV = Hx*Vx + Hy*Vy + Hz*Vz;
        if(HdotV < 0.0f) HdotV = 0.0f;

        /* Fresnel (Schlick) */
        float omHdotV = 1.0f - HdotV;
        float pow5 = omHdotV * omHdotV; pow5 *= pow5; pow5 *= omHdotV;
        const float Fr = F0_r + (1.0f - F0_r) * pow5;
        const float Fg = F0_g + (1.0f - F0_g) * pow5;
        const float Fb = F0_b + (1.0f - F0_b) * pow5;

        /* Energy-conserving Lambertian diffuse: kD = (1 - F) * (1 - metallic) */
        const float kD_r = (1.0f - Fr) * one_minus_metallic;
        const float kD_g = (1.0f - Fg) * one_minus_metallic;
        const float kD_b = (1.0f - Fb) * one_minus_metallic;

        /* GGX normal distribution */
        float d = NdotH * NdotH * (alphaSq - 1.0f) + 1.0f;
        float D = alphaSq / (d * d + 1e-7f);

        /* Smith Schlick-GGX geometry */
        float GV = NdotV / (NdotV * (1.0f - k) + k);
        float GL = NdotL / (NdotL * (1.0f - k) + k + 1e-7f);
        float G  = GV * GL;

        /* Cook-Torrance specular factor */
        float denom = 4.0f * NdotV * NdotL;
        if(denom < 0.0001f) denom = 0.0001f;
        float spec = D * G / denom;

        float scale = NdotL * light.intensity * att;
        total_r += (kD_r * base_color[0] + spec * Fr) * scale * light.color[0];
        total_g += (kD_g * base_color[1] + spec * Fg) * scale * light.color[1];
        total_b += (kD_b * base_color[2] + spec * Fb) * scale * light.color[2];
    }

    out[0] = total_r > 1.0f ? 1.0f : total_r;
    out[1] = total_g > 1.0f ? 1.0f : total_g;
    out[2] = total_b > 1.0f ? 1.0f : total_b;
    out[3] = base_color[3];
}

} // namespace smlt
