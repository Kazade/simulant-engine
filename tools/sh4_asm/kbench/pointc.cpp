/* The C++ point-light pass that pvr_light_point_sh4 replaced (light_pass<true>
 * in pvr_render_queue_visitor.cpp), kept as the reference: kbench checks the
 * asm kernel's weights against it and times both. */
#include <cstdint>
#include "sh4zam/shz_sh4zam.h"

enum : uint32_t {
    LV_NDOTP, LV_NX, LV_NY, LV_NZ, LV_INVV, LV_PX, LV_PY, LV_PZ, LV_W,
    LV_STRIDE = 16,
};

struct PointParams {
    float v[3];         /* eye-space light position */
    float inv_range;
    float bp_n, bp_nm1, bp_scale;
};

extern "C" __attribute__((noinline))
void point_light_c(float (*lit_window_)[16], uint32_t n, const PointParams* L, int li) {
    const float Lx = L->v[0], Ly = L->v[1], Lz = L->v[2];
    const float inv_range = L->inv_range;
    const float sp_n = L->bp_n, sp_nm1 = L->bp_nm1, sp_scale = L->bp_scale;
    const uint32_t wo = LV_W + 2 * li;

    for(uint32_t i = 0; i < n; ++i) {
        float* row = lit_window_[i];
        const float px = row[LV_PX], py = row[LV_PY], pz = row[LV_PZ];

        const float lx = Lx - px, ly = Ly - py, lz = Lz - pz;
        const float pl = px * lx + py * ly + pz * lz;
        const float LL = lx * lx + ly * ly + lz * lz;
        const float invL = shz_inv_sqrtf_fsrra(LL + 1e-18f);
        const float att = 1.0f - LL * invL * inv_range;
        const float NdotL = (row[LV_NX] * lx + row[LV_NY] * ly + row[LV_NZ] * lz) * invL;
        const float LdotV = -pl * invL * row[LV_INVV];
        const float s = NdotL * att;
        const bool lit = (att > 0.0f) && (NdotL > 0.0f);

        float wd = 0.0f, ws = 0.0f;
        if(lit) {
            const float u = 1.0f + LdotV;
            const float rh = shz_inv_sqrtf_fsrra(shz_fabsf(u + u) + 1e-18f);
            float NdotH = (NdotL - row[LV_NDOTP]) * rh;
            if(NdotH < 0.0f) NdotH = 0.0f;
            const float lobe = NdotH * shz_invf_fsrra(sp_n - sp_nm1 * NdotH);
            wd = s;
            ws = s * lobe * sp_scale;
        }

        row[wo + 0] = wd + wd;
        row[wo + 1] = ws + ws;
    }
}
