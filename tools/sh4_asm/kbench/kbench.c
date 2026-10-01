/* Per-kernel cycle microbenchmark for pvr_lighting_sh4.s (see run.sh).
 *
 * Each kernel runs over warm buffers for two vertex counts; the difference
 * divided by the count difference is the steady-state cycles per vertex,
 * with call, prologue and epilogue costs cancelled. Best of several runs. */
#include <kos.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <dc/perfctr.h>

typedef struct { const uint8_t* pos; const uint8_t* nrm; uint32_t stride, n; float* out; float sqrt_eps; } GeomArgs;
typedef struct {
    const uint8_t* pos; const uint8_t* uv; const uint8_t* col; const float* lit;
    void* out; uint32_t n; int32_t pos_step, uv_step, col_step, lit_step;
    float material[4]; float limit;
} Pass1Args;

extern void pvr_light_geometry_sh4(const GeomArgs*);
extern void pvr_light_dir_sh4(float* rows, float* w, uint32_t n, const float* k);
extern void pvr_light_combine2_sh4(float* w, uint32_t n);
extern void pvr_light_combine1_sh4(float* w, uint32_t n);
extern uint32_t pvr_pass1_sh4(const Pass1Args*);
#ifdef PACK32
extern void pvr_pack_sh4(const void* in, void* out, uint32_t n, const float* k);
#define PACK_STRIDE 32
#else   /* the older pack kernel, which also wrote a 32-byte clip entry per vertex */
extern void pvr_pack_sh4(const void* in, void* out, uint32_t n, const float* k, void* clip);
#define PACK_STRIDE 64
#endif

typedef struct { float v[3]; float inv_range; float bp_n, bp_nm1, bp_scale; } PointParams;
extern void point_light_c(float (*rows)[16], uint32_t n, const PointParams* L, int li);
#ifdef HAVE_POINT_ASM
extern void pvr_light_point_sh4(float* rows, float* w, uint32_t n, const float* k);
#endif

#define NMAX 64
#define STRIDE 48
static uint8_t src[NMAX * STRIDE] __attribute__((aligned(32)));
static float rows[NMAX][16] __attribute__((aligned(32)));
static float cv[NMAX][16] __attribute__((aligned(32)));      /* ClipVertex, 64B */
static uint8_t work[NMAX * 64 + 64] __attribute__((aligned(32)));

static void load_xmtrx(const float* m) {
    __asm__ volatile(
        "frchg\n"
        "fmov.s @%0+, fr0\n fmov.s @%0+, fr1\n fmov.s @%0+, fr2\n fmov.s @%0+, fr3\n"
        "fmov.s @%0+, fr4\n fmov.s @%0+, fr5\n fmov.s @%0+, fr6\n fmov.s @%0+, fr7\n"
        "fmov.s @%0+, fr8\n fmov.s @%0+, fr9\n fmov.s @%0+, fr10\n fmov.s @%0+, fr11\n"
        "fmov.s @%0+, fr12\n fmov.s @%0+, fr13\n fmov.s @%0+, fr14\n fmov.s @%0+, fr15\n"
        "frchg\n" : "+r"(m) :: "memory");
}

static const float ident[16] __attribute__((aligned(8))) = {
    1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, -5, 1};

static void fill(void) {
    for(int i = 0; i < NMAX; ++i) {
        float* v = (float*)(src + i * STRIDE);
        v[0] = 1.0f + i * 0.01f; v[1] = 2.0f; v[2] = 3.0f;      /* pos */
        v[3] = 0.0f; v[4] = 0.6f; v[5] = 0.8f;                  /* nrm */
        v[6] = 0.25f; v[7] = 0.75f;                              /* uv */
        v[8] = 0.5f; v[9] = 0.6f; v[10] = 0.7f; v[11] = 1.0f;   /* col */
        /* an eye-space scratch row: (N.P/|P|, N, 1/|P|, P), most lit by
         * the point light below */
        float px = -1.0f + i * 0.03f, py = 0.5f, pz = -5.0f;
        float il = 1.0f / sqrtf(px * px + py * py + pz * pz);
        float nx = 0.0f, ny = 0.6f, nz = 0.8f;
        rows[i][0] = (nx * px + ny * py + nz * pz) * il;
        rows[i][1] = nx; rows[i][2] = ny; rows[i][3] = nz;
        rows[i][4] = il;
        rows[i][5] = px; rows[i][6] = py; rows[i][7] = pz;
        for(int k = 8; k < 16; ++k) rows[i][k] = 0.0f;
        for(int k = 0; k < 16; ++k) cv[i][k] = 0.5f;
        cv[i][2] = 1.0f; cv[i][3] = 2.0f;                       /* z, w */
        ((uint32_t*)cv[i])[13] = 1;                             /* ok */
    }
}

static GeomArgs ga;
static Pass1Args pa;
static const float dk[8] __attribute__((aligned(8))) = {
    0.0f, -0.3f, -0.8f, -0.52f, 1.000004f, 0.5f, -8.0f, 0.01f};
static const float pk[4] __attribute__((aligned(8))) = {320.0f, 240.0f, 127.5f, -127.5f};
static const PointParams pp = {{0.0f, 2.0f, -3.0f}, 0.1f, 32.0f, 31.0f, 4.25f};
#ifdef HAVE_POINT_ASM
static float ptk[24] __attribute__((aligned(8)));

/* The asm kernel's constants for a point light (see pvr_light_point_sh4). */
static void point_k(float* k, const PointParams* L) {
    const float sqrt8 = 2.8284271f;
    memset(k, 0, 16 * sizeof(float));
    k[0] = 1e-9f; k[1] = -L->v[0]; k[2] = -L->v[1]; k[3] = -L->v[2];
    k[5] = 1.0f; k[10] = 1.0f; k[15] = 1.0f;
    k[16] = 1.0f + 4e-6f;
    k[17] = L->inv_range * 0.5f;
    k[18] = 0.5f;
    k[19] = L->bp_nm1 / sqrt8;
    k[20] = -L->bp_n;
    k[21] = L->bp_scale / sqrt8;
}

/* Rows covering every case the C version branches on: facing toward and
 * away from the light, in and out of range, light on either side. */
static void fill_varied(void) {
    for(int i = 0; i < NMAX; ++i) {
        float px = -4.0f + i * 0.13f, py = (i % 5) - 2.0f, pz = -2.0f - (i % 7) * 2.5f;
        float il = 1.0f / sqrtf(px * px + py * py + pz * pz);
        float nx = sinf(i * 0.7f), ny = cosf(i * 1.3f), nz = sinf(i * 0.3f + 1.0f);
        float nl = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
        nx *= nl; ny *= nl; nz *= nl;
        rows[i][0] = (nx * px + ny * py + nz * pz) * il;
        rows[i][1] = nx; rows[i][2] = ny; rows[i][3] = nz;
        rows[i][4] = il;
        rows[i][5] = px; rows[i][6] = py; rows[i][7] = pz;
        for(int k = 8; k < 16; ++k) rows[i][k] = 0.0f;
    }
}

static float ref[NMAX][2];

static void check_point(void) {
    fill_varied();
    point_light_c(rows, NMAX, &pp, 0);
    for(int i = 0; i < NMAX; ++i) { ref[i][0] = rows[i][8]; ref[i][1] = rows[i][9]; }
    fill_varied();
    pvr_light_point_sh4(rows[0], rows[0] + 8, NMAX, ptk);
    float md = 0.0f, ms = 0.0f, maxd = 0.0f, maxs = 0.0f;
    int lit = 0;
    for(int i = 0; i < NMAX; ++i) {
        float dd = fabsf(rows[i][8] - ref[i][0]), ds = fabsf(rows[i][9] - ref[i][1]);
        if(dd > md) md = dd;
        if(ds > ms) ms = ds;
        if(ref[i][0] > maxd) maxd = ref[i][0];
        if(ref[i][1] > maxs) maxs = ref[i][1];
        if(ref[i][0] > 0.0f) ++lit;
    }
    printf("[kbench] point check: %d/%d lit; max |diff| diffuse %g (max %g), specular %g (max %g)\n",
           lit, NMAX, (double)md, (double)maxd, (double)ms, (double)maxs);
}
#endif

static void run(int which, uint32_t n) {
    switch(which) {
        case 0: ga.n = n; load_xmtrx(ident); pvr_light_geometry_sh4(&ga); break;
        case 1: pvr_light_dir_sh4(rows[0], rows[0] + 8, n, dk); break;
        case 2: load_xmtrx(ident); pvr_light_combine2_sh4(rows[0] + 8, n); break;
        case 3: load_xmtrx(ident); pvr_light_combine1_sh4(rows[0] + 8, n); break;
        case 4: pa.n = n; load_xmtrx(ident); pvr_pass1_sh4(&pa); break;
#ifdef PACK32
        case 5: pvr_pack_sh4(cv, work, n, pk); break;
#else
        case 5: pvr_pack_sh4(cv, work, n, pk, work + 32); break;
#endif
        case 6: point_light_c(rows, n, &pp, 0); break;
#ifdef HAVE_POINT_ASM
        case 7: pvr_light_point_sh4(rows[0], rows[0] + 8, n, ptk); break;
#endif
    }
}

static uint64_t time_one(int which, uint32_t n) {
    uint64_t best = ~0ull;
    for(int rep = 0; rep < 50; ++rep) {
        fill();
        run(which, n);          /* warm caches and code */
        fill();
        run(which, n);
        uint64_t t0 = perf_cntr_count(PRFC0);
        run(which, n);
        uint64_t t1 = perf_cntr_count(PRFC0);
        if(t1 - t0 < best) best = t1 - t0;
    }
    return best;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    ga.pos = src; ga.nrm = src + 12; ga.stride = STRIDE; ga.out = rows[0]; ga.sqrt_eps = 1e-9f;
    pa.pos = src; pa.uv = src + 24; pa.col = src + 32; pa.lit = rows[0] + 8; pa.out = cv;
    pa.pos_step = STRIDE - 8; pa.uv_step = STRIDE - 4; pa.col_step = STRIDE - 12;
    pa.lit_step = 16 * 4 - 24;
    for(int k = 0; k < 4; ++k) pa.material[k] = 1.0f;
    pa.limit = 1e36f;

    static const char* names[] = {"geo", "dir", "comb2", "comb1", "p1", "pk", "pointC", "point"};
#ifdef HAVE_POINT_ASM
    point_k(ptk, &pp);
    check_point();
    const int nk = 8;
#else
    const int nk = 7;
#endif
    const uint32_t lo = 16, hi = 48;   /* both even: whole kernel passes */
    for(int w = 0; w < nk; ++w) {
        uint64_t a = time_one(w, lo), b = time_one(w, hi);
        printf("[kbench] %-6s %7.2f cycles/vertex  (n=%u: %llu, n=%u: %llu)\n", names[w],
               (double)(b - a) / (double)(hi - lo), (unsigned)lo,
               (unsigned long long)a, (unsigned)hi, (unsigned long long)b);
    }
    /* Pack's output for a fixed window, to compare kernels word for word */
    fill();
    for(int i = 0; i < 8; ++i) {
        cv[i][0] = -1.0f + i * 0.4f; cv[i][1] = 0.5f - i * 0.2f; cv[i][2] = 0.3f * i - 1.5f;
        cv[i][3] = 1.0f + i * 0.5f;
        cv[i][6] = 0.1f * i; cv[i][7] = 1.2f - 0.15f * i; cv[i][8] = 0.33f; cv[i][9] = 0.9f;
        cv[i][10] = 0.05f * i; cv[i][11] = 0.07f; cv[i][12] = 0.5f;
        ((uint32_t*)cv[i])[13] = (i == 5) ? 0 : 1;
    }
    run(5, 8);
    for(int i = 0; i < 8; ++i) {
        const uint32_t* w = (const uint32_t*)(work + i * PACK_STRIDE);
        printf("[kbench] pk %d: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n", i,
               (unsigned long)w[0], (unsigned long)w[1], (unsigned long)w[2], (unsigned long)w[3],
               (unsigned long)w[4], (unsigned long)w[5], (unsigned long)w[6], (unsigned long)w[7]);
    }
    printf("[kbench] done\n");
    return 0;
}
