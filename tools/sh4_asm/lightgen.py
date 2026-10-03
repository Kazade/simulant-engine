"""Generate simulant/renderers/pvr/pvr_lighting_sh4.s.

    python tools/sh4_asm/lightgen.py > simulant/renderers/pvr/pvr_lighting_sh4.s

Schedules are cached in tools/sh4_asm/*_ii<II>.json; re-solving (after changing
an op list or an II) needs OR-Tools (pip install ortools). search_ii.py finds
the smallest II each kernel can be scheduled at.
"""
import os
from modsched import LOAD, FLD, FTRV, FIPR, FSRRA, FMUL, FMUL_ST, MOV, cached_solve, gen_loop

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Geometry pass
# ---------------------------------------------------------------------------
GEO_II = 23
GEO_OPS = [
    ("L0", "LS", "fmov.s  @{rp}+, {f0}", []),
    ("L1", "LS", "fmov.s  @{rp}+, {f1}", [("L0", 1)]),
    ("L2", "LS", "fmov.s  @{rp}, {f2}", [("L1", 1)]),
    ("F1", "LS", "fldi1   {f3}", []),
    ("A0", "EX", "add     r6, {rp}", [("L2", 1)]),
    ("L3", "LS", "fmov.s  @{rn}+, {f4}", []),
    ("L4", "LS", "fmov.s  @{rn}+, {f5}", [("L3", 1)]),
    ("L5", "LS", "fmov.s  @{rn}, {f6}", [("L4", 1)]),
    ("FU", "LS", "fsts    fpul, {f7}", []),
    ("A1", "EX", "add     r6, {rn}", [("L5", 1)]),
    ("TP", "FE", "ftrv    xmtrx, {v0}", [("L0", LOAD), ("L1", LOAD), ("L2", LOAD), ("F1", FLD)]),
    ("TN", "FE", "ftrv    xmtrx, {v4}", [("L3", LOAD), ("L4", LOAD), ("L5", LOAD), ("FU", FLD)]),
    # Claim both of the scratch row's lines (MOVCA.L stores r0): the second
    # (slots 8-15: light weights, the light passes' scratch, the combine's
    # output) is only ever written before it's read, by the passes that
    # follow, and the first P store overwrites the word written in the first.
    ("MC2", "LS", "movca.l r0, @{ro}", []),
    ("A4", "EX", "add     #-4, {ro}", [("MC2", 1)]),
    ("MC", "LS", "movca.l r0, @{ro}", [("A4", 1)]),
    ("S2", "LS", "fmov.s  {f2}, @{ro}", [("TP", FTRV), ("MC", 1)]),
    ("S1", "LS", "fmov.s  {f1}, @-{ro}", [("TP", FTRV), ("S2", 1)]),
    ("S0", "LS", "fmov.s  {f0}, @-{ro}", [("TP", FTRV), ("S1", 1)]),
    ("NN", "FE", "fipr    {v4}, {v4}", [("TN", FTRV)]),
    ("PP", "FE", "fipr    {v0}, {v0}", [("TP", FTRV)]),
    ("RV", "FE", "fsrra   {f3}", [("PP", FIPR)]),
    ("RN", "FE", "fsrra   {f7}", [("NN", FIPR)]),
    ("SV", "LS", "fmov.s  {f3}, @-{ro}", [("RV", FSRRA), ("S0", 1)]),
    ("MZ", "FE", "fmul    {f7}, {f6}", [("RN", FSRRA)]),
    ("MY", "FE", "fmul    {f7}, {f5}", [("RN", FSRRA)]),
    ("MX", "FE", "fmul    {f7}, {f4}", [("RN", FSRRA)]),
    ("Z7", "LS", "fldi0   {f7}", [("MX", 1), ("MY", 1), ("MZ", 1)]),
    ("NP", "FE", "fipr    {v0}, {v4}", [("Z7", FLD), ("MX", FMUL), ("MY", FMUL), ("MZ", FMUL), ("PP", 1)]),
    ("SZ", "LS", "fmov.s  {f6}, @-{ro}", [("MZ", FMUL_ST), ("SV", 1)]),
    ("SY", "LS", "fmov.s  {f5}, @-{ro}", [("MY", FMUL_ST), ("SZ", 1)]),
    ("SX", "LS", "fmov.s  {f4}, @-{ro}", [("MX", FMUL_ST), ("SY", 1)]),
    ("M0", "FE", "fmul    {f3}, {f7}", [("NP", FIPR), ("RV", FSRRA)]),
    ("SD", "LS", "fmov.s  {f7}, @-{ro}", [("M0", FMUL_ST), ("SX", 1)]),
    ("AO", "EX", "add     r4, {ro}", [("SD", 1)]),
]
GEO_FE_BUSY = {"TP": 4, "TN": 4, "RV": 3, "RN": 3}
GEO_REUSE = [
    ("L0", ["S0", "PP", "NP"]), ("L1", ["S1", "PP", "NP"]), ("L2", ["S2", "PP", "NP"]),
    ("F1", ["SV", "M0", "RV", "NP"]),                # fr3: 1, P.w, P.P, 1/|P|
    ("L3", ["SX", "MX", "NP"]), ("L4", ["SY", "MY", "NP"]), ("L5", ["SZ", "MZ", "NP"]),
    ("FU", ["SD", "M0", "NP", "MX", "MY", "MZ"]),    # fr7
    ("MC2", ["AO"]),                                 # out pointer
    ("L0", ["A0"]), ("L3", ["A1"]),                  # source pointers
]
GEO_SETS = {
    0: dict(rp="r2", rn="r3", ro="r1", **{f"f{i}": f"fr{i}" for i in range(8)}, v0="fv0", v4="fv4"),
    1: dict(rp="r5", rn="r9", ro="r8", **{f"f{i}": f"fr{i + 8}" for i in range(8)}, v0="fv8", v4="fv12"),
}


def geometry():
    sol = cached_solve("geo", GEO_OPS, GEO_FE_BUSY, GEO_REUSE, GEO_II, sets=GEO_SETS[0])
    pro, ker, epi, single, ip = gen_loop(
        GEO_OPS, GEO_SETS, sol, GEO_II, "geo",
        single_subst=[("add     r6,", "add     r11,"), ("add     r4, r1", "add     #96, r1"),])
    assert ip == 1
    L = "\n".join
    return f"""
!
! void pvr_light_geometry_sh4(const PvrGeomArgs* args)
!
! r4 : args, laid out as
!        +0  const uint8_t* pos     first vertex's position (3 floats)
!        +4  const uint8_t* nrm     first vertex's normal (3 floats)
!        +8  uint32_t stride        source vertex stride in bytes
!        +12 uint32_t n             vertex count
!        +16 float* out             scratch window, 16 floats per vertex
!        +20 float sqrt_eps         s, see below
!
! On entry XMTRX holds the modelview with its fourth row replaced by
! (0, 0, 0, s). For each vertex writes (P.z first, then back to front)
!   out[7..5]  eye-space P (z, y, x)
!   out[4]     1 / |P|
!   out[3..1]  unit eye-space N (z, y, x)
!   out[0]     N.P / |P|  (= -N.V, eye at the origin)
!
! The FSRRA biases come for free from the w lanes: P = MV (p, 1) has
! P.w = s, so FIPR(P, P) = |P|^2 + s^2; N = MV (n, s) has N.w = s^2, so
! FIPR(N, N) = |N|^2 + s^4. (n's w of s adds s * translation to N, which is
! ~1e-9 relative.) N.P is taken after N is normalised and its w lane zeroed,
! so the reciprocal square roots never wait on it.
!
! Scheduling: one vertex starts every {GEO_II} cycles, alternating between two
! register sets (a: fr0-7, source r2/r3, out r1; b: fr8-15, r5/r9, r8) so
! each may stay live for {2 * GEO_II}. The FP pipe sets the pace: after an FTRV
! it takes no vector op (FIPR / FSRRA / FTRV) for 4 cycles and no scalar
! arithmetic for 5, after an FSRRA 3 and 4; the LS ops dual-issue alongside.
!
! Both of each scratch row's lines are claimed with MOVCA.L before they are
! written, so writing them never reads them from RAM first (the window is
! evicted between batches by the vertex data streaming through the
! direct-mapped cache). Unclaimed, the second line cost the following light
! pass ~15 cycles a row in data cache stalls.
!
! n < 2 and an odd final vertex go through the unpipelined copy at the end.
!
    .section .text._pvr_light_geometry_sh4, "ax", %progbits
    .globl _pvr_light_geometry_sh4
    .align 5
_pvr_light_geometry_sh4:
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr15, @-r15
    mov.l   @r4+, r2            ! pos
    mov.l   @r4+, r3            ! nrm
    mov.l   @r4+, r6            ! stride
    mov.l   @r4+, r7            ! n
    mov.l   @r4+, r1            ! out
    lds.l   @r4+, fpul          ! sqrt_eps
    mov     r6, r11
    add     #-8, r11            ! single-vertex source step
    mov     r2, r5
    add     r6, r5              ! b: pos of vertex 1
    mov     r3, r9
    add     r6, r9              ! b: nrm of vertex 1
    mov     r1, r8
    add     #96, r8             ! b: row 1, +32
    add     #32, r1             ! a: row 0, +32
    add     r6, r6
    add     #-8, r6             ! pair source step
    mov     #80, r4
    add     r4, r4              ! 160: out step, past the other set's row
    shlr    r7                  ! pairs; T = n & 1
    movt    r10                 ! single-vertex tail count (all of n if n < 2)
    tst     r7, r7
    bt      .geo_single
{L(pro)}
    dt      r7                  ! kernel iterations = pairs - 1
    bt      .geo_epilogue
    .align 5
.geo_kernel:
{L(ker)}
.geo_epilogue:
{L(epi)}
.geo_single:
    tst     r10, r10
    bt      .geo_done
{L(single)}
.geo_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    rts
    mov.l   @r15+, r8
"""


# ---------------------------------------------------------------------------
# Combine pass (dielectric)
# ---------------------------------------------------------------------------
COMB_II = {2: 11, 1: 9}


def comb_ops(nlights):
    """Scratch row -> scratch row: reads the weights at +32..+44, writes
    D (ambient + diffuse) to +32..+40 and S (specular) to +48..+56."""
    two = nlights == 2
    ops = [("LD0", "LS", "fmov.s  @{rw}+, {d0}", [])]
    if two:
        ops += [
            ("LS0", "LS", "fmov.s  @{rw}+, {s0}", [("LD0", 1)]),
            ("LD1", "LS", "fmov.s  @{rw}+, {d1}", [("LS0", 1)]),
            ("LS1", "LS", "fmov.s  @{rw}, {s1}", [("LD1", 1)]),
            ("TD", "FE", "ftrv    xmtrx, {vD}", [("LD0", LOAD), ("LD1", LOAD)]),
            ("TS", "FE", "ftrv    xmtrx, {vS}", [("LS0", LOAD), ("LS1", LOAD)]),
        ]
        last = "LS1"
    else:
        ops += [
            ("LS0", "LS", "fmov.s  @{rw}, {s0}", [("LD0", 1)]),
            ("A8", "EX", "add     #8, {rw}", [("LS0", 1)]),
            ("TD", "FE", "ftrv    xmtrx, {vD}", [("LD0", LOAD)]),
            ("TS", "FE", "ftrv    xmtrx, {vS}", [("LS0", LOAD)]),
        ]
        last = "A8"
    ops += [
        ("DB", "LS", "fmov.s  {d2}, @-{rw}", [("TD", FTRV), (last, 1)]),
        ("DG", "LS", "fmov.s  {d1}, @-{rw}", [("TD", FTRV), ("DB", 1)]),
        ("DR", "LS", "fmov.s  {d0}, @-{rw}", [("TD", FTRV), ("DG", 1)]),
        ("A28", "EX", "add     #28, {rw}", [("DR", 1)]),
        ("SB", "LS", "fmov.s  {s2}, @-{rw}", [("TS", FTRV), ("A28", 1)]),
        ("SG", "LS", "fmov.s  {s1}, @-{rw}", [("TS", FTRV), ("SB", 1)]),
        ("SR", "LS", "fmov.s  {s0}, @-{rw}", [("TS", FTRV), ("SG", 1)]),
        ("AW", "EX", "add     #112, {rw}", [("SR", 1)]),
    ]
    reuse = [
        ("LD0", ["DR", "AW"]), ("LS0", ["SR"]),
        ("TD", ["DB", "DG", "DR"]), ("TS", ["SB", "SG", "SR"]),
    ]
    if two:
        reuse += [("LD1", ["DG"]), ("LS1", ["SG"])]
    return ops, {"TD": 4, "TS": 4}, reuse


COMB_SETS = {
    0: dict(rw="r1", d0="fr0", d1="fr1", d2="fr2", s0="fr4", s1="fr5", s2="fr6", vD="fv0", vS="fv4"),
    1: dict(rw="r5", d0="fr8", d1="fr9", d2="fr10", s0="fr12", s1="fr13", s2="fr14", vD="fv8", vS="fv12"),
}


def combine(nlights):
    ops, busy, reuse = comb_ops(nlights)
    II = COMB_II[nlights]
    sol = cached_solve(f"comb{nlights}", ops, busy, reuse, II, sets=COMB_SETS[0])
    lab = f"comb{nlights}"
    pro, ker, epi, single, ip = gen_loop(ops, COMB_SETS, sol, II, lab,
                                         single_subst=[("add     #112,", "add     #48,")])
    assert ip == 1
    L = "\n".join
    zero1 = "" if nlights == 2 else """
    fldi0   fr1                 ! light 1's lanes: never loaded, times a
    fldi0   fr5                 ! zero column
    fldi0   fr9
    fldi0   fr13"""
    return f"""
!
! void pvr_light_combine{nlights}_sh4(float* w, uint32_t n)
!
! r4 : the first scratch row's light weights: wd0, ws0, wd1, ws1 at +0..+12
! r5 : vertex count
!
! On entry XMTRX holds column 0 = light 0's colour, column 1 = light 1's,
! column 2 = 0, column 3 = (ambient, 1). Then with D = (wd0, wd1, -, 1) and
! S = (ws0, ws1, -, 0)
!   FTRV(D) = (ambient + sum(wd * colour), 1)
!   FTRV(S) = (sum(ws * colour), 0)
! which replace the weights in each row: D's rgb at +0..+8 and S's at
! +16..+24 (64-byte row stride). Row 3 of the matrix keeps the w lanes at 1
! and 0 and column 2 ignores lane 2, so after the setup below neither needs
! rewriting per vertex.
!
! {II} cycles per vertex (the two FTRVs hold FE for 8), pipelined as in the
! geometry pass (a: fr0-7, row pointer r1; b: fr8-15, r5).
!
    .section .text._pvr_light_combine{nlights}_sh4, "ax", %progbits
    .globl _pvr_light_combine{nlights}_sh4
    .align 5
_pvr_light_combine{nlights}_sh4:
    fmov.s  fr12, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr15, @-r15
    mov     r5, r7
    mov     r4, r1              ! a: row 0
    mov     r4, r5
    add     #64, r5             ! b: row 1
    fldi1   fr3                 ! D.w
    fldi0   fr7                 ! S.w
    fldi1   fr11
    fldi0   fr15
    fldi0   fr2                 ! lane 2 meets a zero column; just keep it finite
    fldi0   fr6
    fldi0   fr10
    fldi0   fr14{zero1}
    shlr    r7                  ! pairs; T = n & 1
    movt    r6                  ! single-vertex tail count (all of n if n < 2)
    tst     r7, r7
    bt      .{lab}_single
{L(pro)}
    dt      r7                  ! kernel iterations = pairs - 1
    bt      .{lab}_epilogue
    .align 5
.{lab}_kernel:
{L(ker)}
.{lab}_epilogue:
{L(epi)}
.{lab}_single:
    tst     r6, r6
    bt      .{lab}_done
{L(single)}
.{lab}_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    rts
    fmov.s  @r15+, fr12
"""


# ---------------------------------------------------------------------------
# Pass 1: clip-space position, UV, colour -> ClipVertex
# ---------------------------------------------------------------------------
P1_II = 32
P1_OPS = [
    # position -> clip (XMTRX = MVP)
    ("L0", "LS", "fmov.s  @r1+, fr0", []),
    ("L1", "LS", "fmov.s  @r1+, fr1", [("L0", 1)]),
    ("L2", "LS", "fmov.s  @r1, fr2", [("L1", 1)]),
    ("F1", "LS", "fldi1   fr3", []),
    ("AP", "EX", "add     r8, r1", [("L2", 1)]),
    ("TR", "FE", "ftrv    xmtrx, fv0", [("L0", LOAD), ("L1", LOAD), ("L2", LOAD), ("F1", FLD)]),
    # lit row: D (lit colour factor) at +0, S (specular) at +16
    ("LDR", "LS", "fmov.s  @r5+, fr7", []),
    ("LDG", "LS", "fmov.s  @r5+, fr8", [("LDR", 1)]),
    ("LDB", "LS", "fmov.s  @r5+, fr9", [("LDG", 1)]),
    ("AL4", "EX", "add     #4, r5", [("LDB", 1)]),
    ("LSR", "LS", "fmov.s  @r5+, fr4", [("AL4", 1)]),
    ("LSG", "LS", "fmov.s  @r5+, fr5", [("LSR", 1)]),
    ("LSB", "LS", "fmov.s  @r5, fr6", [("LSG", 1)]),
    ("ALS", "EX", "add     r12, r5", [("LSB", 1)]),
    # ClipVertex stores run back to front from +52: sb sg sr a b g r v u w z y x.
    # Its two lines are claimed with MOVCA.L first (it stores r0, which the
    # ok and g stores overwrite).
    ("M1", "LS", "movca.l r0, @r6", []),
    ("WSB", "LS", "fmov.s  fr6, @-r6", [("LSB", LOAD), ("M1", 1)]),
    ("WSG", "LS", "fmov.s  fr5, @-r6", [("LSG", LOAD), ("WSB", 1)]),
    ("WSR", "LS", "fmov.s  fr4, @-r6", [("LSR", LOAD), ("WSG", 1)]),
    # colour (4 floats) x material x D, alpha x material
    ("LCR", "LS", "fmov.s  @r3+, fr4", [("WSR", 1)]),
    ("LCG", "LS", "fmov.s  @r3+, fr5", [("LCR", 1), ("WSG", 1)]),
    ("LCB", "LS", "fmov.s  @r3+, fr6", [("LCG", 1), ("WSB", 1)]),
    ("LCA", "LS", "fmov.s  @r3, fr10", [("LCB", 1)]),
    ("AC", "EX", "add     r10, r3", [("LCA", 1)]),
    ("MA", "FE", "fmul    fr15, fr10", [("LCA", LOAD)]),
    ("MR1", "FE", "fmul    fr12, fr4", [("LCR", LOAD)]),
    ("MG1", "FE", "fmul    fr13, fr5", [("LCG", LOAD)]),
    ("MB1", "FE", "fmul    fr14, fr6", [("LCB", LOAD)]),
    ("MR2", "FE", "fmul    fr7, fr4", [("MR1", FMUL), ("LDR", LOAD)]),
    ("MG2", "FE", "fmul    fr8, fr5", [("MG1", FMUL), ("LDG", LOAD)]),
    ("MB2", "FE", "fmul    fr9, fr6", [("MB1", FMUL), ("LDB", LOAD)]),
    ("WA", "LS", "fmov.s  fr10, @-r6", [("MA", FMUL_ST), ("WSR", 1)]),
    ("WB", "LS", "fmov.s  fr6, @-r6", [("MB2", FMUL_ST), ("WA", 1)]),
    ("D4", "EX", "add     #-4, r6", [("WB", 1)]),
    ("M0", "LS", "movca.l r0, @r6", [("D4", 1)]),
    ("WG", "LS", "fmov.s  fr5, @r6", [("MG2", FMUL_ST), ("M0", 1)]),
    ("WR", "LS", "fmov.s  fr4, @-r6", [("MR2", FMUL_ST), ("WG", 1)]),
    # UV copy
    ("LU", "LS", "fmov.s  @r2+, fr7", [("MR2", 1)]),
    ("LV", "LS", "fmov.s  @r2, fr8", [("LU", 1), ("MG2", 1)]),
    ("AU", "EX", "add     r9, r2", [("LV", 1)]),
    ("WV", "LS", "fmov.s  fr8, @-r6", [("LV", LOAD), ("WR", 1)]),
    ("WU", "LS", "fmov.s  fr7, @-r6", [("LU", LOAD), ("WV", 1)]),
    # clip position, then validity: x^2 + y^2 + z^2 + w^2 < 1e36 is false
    # for NaN and Inf in any lane
    ("WW", "LS", "fmov.s  fr3, @-r6", [("TR", FTRV), ("WU", 1)]),
    ("WZ", "LS", "fmov.s  fr2, @-r6", [("TR", FTRV), ("WW", 1)]),
    ("WY", "LS", "fmov.s  fr1, @-r6", [("TR", FTRV), ("WZ", 1)]),
    ("WX", "LS", "fmov.s  fr0, @-r6", [("TR", FTRV), ("WY", 1)]),
    ("VF", "FE", "fipr    fv0, fv0", [("TR", FTRV), ("WW", 1)]),
    ("FC", "FE", "fcmp/gt fr3, fr11", [("VF", FIPR)]),
    ("MT", "EX", "movt    r0", [("FC", 1)]),
    ("OK", "LS", "mov.l   r0, @(52, r6)", [("MT", 1), ("WX", 1)]),
    ("AV", "EX", "add     r0, r4", [("MT", 1)]),
    ("AO", "EX", "add     #116, r6", [("OK", 1)]),
]
P1_FE_BUSY = {"TR": 4}
# One register set, so each register's lifetime must fit in II.
P1_REUSE = [
    ("L0", ["WX", "VF"]), ("L1", ["WY", "VF"]), ("L2", ["WZ", "VF"]),
    ("F1", ["WW", "VF", "FC"]),
    ("LSR", ["WR"]), ("LSG", ["WG"]), ("LSB", ["WB"]),
    ("LDR", ["WU", "MR2"]), ("LDG", ["WV", "MG2"]), ("LDB", ["MB2"]),
    ("LCA", ["WA"]),
    ("L0", ["AP"]), ("LDR", ["ALS"]), ("LCR", ["AC"]), ("LU", ["AU"]),
    ("M1", ["AO", "OK"]),
    ("MT", ["AV", "OK"]),
]


def p1_extra(m, t, c, II):
    # FC and MT share T with the loop's dt: keep them in one kernel pass,
    # with room for dt after them.
    m.Add(c["MT"] > c["FC"])
    m.Add(c["MT"] <= II - 3)


def pass1():
    sol = cached_solve("p1", P1_OPS, P1_FE_BUSY, P1_REUSE, P1_II, nsets=1, extra=p1_extra)
    pro, ker, epi, single, ip = gen_loop(P1_OPS, {0: {}}, sol, P1_II, "p1", dt_after=["MT"])
    L = "\n".join
    return f"""
!
! uint32_t pvr_pass1_sh4(const Pass1AsmArgs* args)
!
! r4 : args, laid out as
!        +0  const uint8_t* pos     first vertex's position (3 floats)
!        +4  const uint8_t* uv      first UV (2 floats)
!        +8  const uint8_t* col     first colour (4 floats)
!        +12 const float* lit       first lit row's D (3 floats; S follows at +16)
!        +16 ClipVertex* out
!        +20 uint32_t n
!        +24 int32_t pos_step       each stream's stride, less what its
!        +28 int32_t uv_step        post-increment loads advance: pos - 8,
!        +32 int32_t col_step       uv - 4, col - 12, lit - 24
!        +36 int32_t lit_step
!        +40 float material[4]
!        +56 float limit            1e36
!
! XMTRX holds MVP. For each vertex writes the ClipVertex's x, y, z, w (clip
! position), u, v, r, g, b (colour x material x D), a (alpha x material),
! sr, sg, sb (S) and ok. A missing attribute is a zero stride over a
! constant; unlit vertices read a constant lit row. Returns the number of
! vertices whose clip position is not finite (ok = 0), which the caller
! replaces.
!
! ClipVertex is 64 bytes, 32-byte aligned: both of its lines are claimed
! with MOVCA.L before being written, as the work array is always cold.
!
! {P1_II} cycles per vertex, bound by the LS pipe (32 loads, stores, fldi
! and MOVCA.L). Software pipelined with one register set: fr0-3 clip position,
! fr4-10 reused in store order, fr11 the limit, fr12-15 the material.
!
    .section .text._pvr_pass1_sh4, "ax", %progbits
    .globl _pvr_pass1_sh4
    .align 5
_pvr_pass1_sh4:
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r12, @-r15
    mov.l   r13, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr15, @-r15
    mov.l   @r4+, r1            ! pos
    mov.l   @r4+, r2            ! uv
    mov.l   @r4+, r3            ! col
    mov.l   @r4+, r5            ! lit
    mov.l   @r4+, r6            ! out
    mov.l   @r4+, r7            ! n
    mov.l   @r4+, r8            ! pos_step
    mov.l   @r4+, r9            ! uv_step
    mov.l   @r4+, r10           ! col_step
    mov.l   @r4+, r12           ! lit_step
    fmov.s  @r4+, fr12          ! material
    fmov.s  @r4+, fr13
    fmov.s  @r4+, fr14
    fmov.s  @r4+, fr15
    fmov.s  @r4+, fr11          ! limit
    add     #52, r6             ! stores run back to front
    mov     r7, r13             ! n, for the return value
    mov     #0, r4              ! valid count
    mov     #{ip}, r0
    cmp/hs  r0, r7
    bf      .p1_single          ! n < {ip}: too short to pipeline
{L(pro)}
    add     #-{ip}, r7          ! kernel iterations = n - {ip}
    tst     r7, r7
    bt      .p1_epilogue
    .align 5
.p1_kernel:
{L(ker)}
.p1_epilogue:
{L(epi)}
    bra     .p1_done
    nop
.p1_single:
    tst     r7, r7
    bt      .p1_done
.p1_single_loop:
{L(single)}
    dt      r7
    bf      .p1_single_loop
.p1_done:
    mov     r13, r0
    sub     r4, r0              ! invalid = n - valid
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr12
    mov.l   @r15+, r13
    mov.l   @r15+, r12
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    rts
    mov.l   @r15+, r8
"""


# ---------------------------------------------------------------------------
# Pack: ClipVertex -> submit-ready PVR vertex (see pack_window in the C++)
# ---------------------------------------------------------------------------
PK_II = 52
FTRC = 4


def pk_ops():
    ops = [
        # ClipVertex loads, in address order: x y z w u v r g b a sr sg sb ok
        ("LX", "LS", "fmov.s  @r1+, fr1", []),
        ("LY", "LS", "fmov.s  @r1+, fr2", [("LX", 1)]),
        ("LZ", "LS", "fmov.s  @r1+, fr3", [("LY", 1)]),
        ("LW", "LS", "fmov.s  @r1+, fr4", [("LZ", 1)]),
        ("LU", "LS", "mov.l   @r1+, r10", [("LW", 1)]),
        ("LV", "LS", "mov.l   @r1+, r11", [("LU", 1)]),
    ]
    # channels: (name, ClipVertex order), temps rotate over three pairs
    chans = ["R", "G", "B", "A", "SR", "SG", "SB"]
    pairs = [("fr6", "fr7"), ("fr8", "fr9"), ("fr10", "fr11")]
    prev = "LV"
    for k, c in enumerate(chans):
        fc, fe = pairs[k % 3]
        deps = [(prev, 1)]
        if k >= 3:
            deps.append((f"T{chans[k - 3]}", 1))       # pair free again
            deps.append((f"S{chans[k - 3]}", 1))
        ops.append((f"L{c}", "LS", f"fmov.s  @r1+, {fc}", deps))
        prev = f"L{c}"
    ops += [
        ("LOK", "LS", "mov.b   @r1, r4", [("LSB", 1)]),
        ("AI", "EX", "add     #12, r1", [("LOK", 1)]),
        # visibility: z + w > 0 and finite
        ("VZ", "FE", "fadd    fr4, fr3", [("LZ", LOAD), ("LW", LOAD)]),
        ("FC", "FE", "fcmp/gt fr15, fr3", [("VZ", 3)]),
        ("MT", "EX", "movt    r5", [("FC", 1)]),
        ("VA", "EX", "and     r4, r5", [("MT", 1), ("LOK", 2)]),
        ("VN", "EX", "neg     r5, r5", [("VA", 1)]),
        ("VF", "EX", "and     r3, r5", [("VN", 1)]),
        # 1/w (w > 0 whenever the vertex is emitted), screen x/y
        ("QC", "LS", "fmov    fr4, fr5", [("LW", LOAD)]),
        ("QM", "FE", "fmul    fr5, fr5", [("QC", 1)]),
        ("QR", "FE", "fsrra   fr5", [("QM", FMUL)]),
        ("XW", "FE", "fadd    fr4, fr1", [("LX", LOAD), ("LW", LOAD)]),
        ("YN", "LS", "fneg    fr2", [("LY", LOAD)]),
        ("YW", "FE", "fadd    fr4, fr2", [("YN", 1), ("LW", LOAD)]),
        ("KX", "FE", "fmul    fr5, fr1", [("QR", FSRRA), ("XW", FMUL)]),
        ("KY", "FE", "fmul    fr5, fr2", [("QR", FSRRA), ("YW", FMUL)]),
        ("HX", "FE", "fmul    fr13, fr1", [("KX", FMUL)]),
        ("HY", "FE", "fmul    fr14, fr2", [("KY", FMUL)]),
    ]
    # per channel: d = 127.5 (c - 1); t = trunc(|d| - d) = 255 (1 - min(c, 1)),
    # converted one after another through FPUL, in load order. r, g, b and
    # sr, sg, sb accumulate a byte at a time; alpha goes in last, shifted up.
    prev_fpul = None
    last = {}          # accumulator -> last op writing it
    prev_r9 = None     # last op reading r9
    for c in chans:
        k = chans.index(c)
        fc, fe = pairs[k % 3]
        ops += [
            (f"M{c}", "FE", f"fmul    fr0, {fc}", [(f"L{c}", LOAD)]),
            (f"D{c}", "FE", f"fadd    fr12, {fc}", [(f"M{c}", FMUL)]),
            (f"C{c}", "LS", f"fmov    {fc}, {fe}", [(f"D{c}", FMUL)]),
            (f"F{c}", "LS", f"fabs    {fe}", [(f"C{c}", 1)]),
            (f"S{c}", "FE", f"fsub    {fc}, {fe}", [(f"F{c}", 1)]),
        ]
        tdeps = [(f"S{c}", FMUL)] + ([(prev_fpul, 1)] if prev_fpul else [])
        ops.append((f"T{c}", "FE", f"ftrc    {fe}, fpul", tdeps))
        accr = "r8" if c.startswith("S") else "r6"
        if c in ("R", "SR"):
            ops.append((f"ST{c}", "LS", f"sts     fpul, {accr}", [(f"T{c}", FTRC)]))
            last[accr] = f"ST{c}"
        elif c == "A":
            sd = [(f"T{c}", FTRC)] + ([(prev_r9, 1)] if prev_r9 else [])
            ops += [
                (f"ST{c}", "LS", "sts     fpul, r9", sd),
                ("SHA1", "EX", "shll16  r9", [(f"ST{c}", 1)]),
                ("SHA2", "EX", "shll8   r9", [("SHA1", 1)]),
                (f"OR{c}", "EX", f"or      r9, {accr}", [("SHA2", 1), (last[accr], 1)]),
            ]
            last[accr] = f"OR{c}"
            prev_r9 = f"OR{c}"
        else:
            sd = [(f"T{c}", FTRC)] + ([(prev_r9, 1)] if prev_r9 else [])
            ops += [
                (f"ST{c}", "LS", "sts     fpul, r9", sd),
                (f"SH{c}", "EX", f"shll8   {accr}", [(last[accr], 1)]),
                (f"OR{c}", "EX", f"or      r9, {accr}", [(f"ST{c}", 1), (f"SH{c}", 1)]),
            ]
            last[accr] = f"OR{c}"
            prev_r9 = f"OR{c}"
        prev_fpul = f"ST{c}"
    ops += [
        ("NA", "EX", "not     r6, r6", [("ORA", 1)]),
        ("NO", "EX", "not     r8, r8", [("ORSB", 1)]),
        # Claim the line (r2 at +24 on entry), store v u z y x flags back to
        # front as soon as they are ready, then the colour words - computed
        # last, through the FPUL chain - by displacement.
        ("MC", "LS", "movca.l r0, @r2", []),
        ("WV", "LS", "mov.l   r11, @-r2", [("MC", 1), ("LV", LOAD)]),
        ("WU", "LS", "mov.l   r10, @-r2", [("WV", 1), ("LU", LOAD)]),
        ("WZ", "LS", "fmov.s  fr5, @-r2", [("WU", 1), ("QR", FSRRA)]),
        ("WY", "LS", "fmov.s  fr2, @-r2", [("WZ", 1), ("HY", FMUL_ST)]),
        ("WX", "LS", "fmov.s  fr1, @-r2", [("WY", 1), ("HX", FMUL_ST)]),
        ("WF", "LS", "mov.l   r5, @-r2", [("WX", 1), ("VF", 1)]),
        ("WA", "LS", "mov.l   r6, @(24, r2)", [("WF", 1), ("NA", 1)]),
        ("WO", "LS", "mov.l   r8, @(28, r2)", [("WF", 1), ("NO", 1)]),
        ("AO", "EX", "add     #56, r2", [("WA", 1), ("WO", 1)]),
        # phantom: reserves the kernel's dt slot, after movt (T-bit)
        ("DTS", "EX", "", [("MT", 1)]),
    ]
    names = [n for n, *_ in ops]
    assert len(names) == len(set(names)), "duplicate op names"
    busy = {"QR": 3}
    # lifetimes within II (one register set)
    reuse = [
        ("LX", ["WX", "HX"]), ("LY", ["WY", "HY"]), ("LZ", ["FC", "VZ"]),
        ("LW", ["QC", "XW", "YW", "VZ"]), ("QC", ["WZ", "KX", "KY"]),
        ("LU", ["WU"]), ("LV", ["WV"]), ("LOK", ["VA"]), ("MT", ["VA"]),
        ("STR", ["NA", "WA"]), ("STSR", ["NO", "WO"]),
        ("LX", ["AI"]), ("MC", ["AO"]),
        ("TR", ["STSB"]),       # FPUL chain of one vertex before the next's
    ]
    for k, c in enumerate(chans):
        reuse.append((f"L{c}", [f"T{c}", f"S{c}"]))
    return ops, busy, reuse


def pk_extra(m, t, c, II):
    m.Add(c["MT"] > c["FC"])
    m.Add(c["DTS"] > c["MT"])
    m.Add(c["DTS"] <= II - 2)    # bf/s and its delay slot follow


def pack():
    ops, busy, reuse = pk_ops()
    sol = cached_solve("pk", ops, busy, reuse, PK_II, nsets=1, extra=pk_extra)
    pro, ker, epi, single, ip = gen_loop(ops, {0: {}}, sol, PK_II, "pk", dt_after=["MT"])
    L = "\n".join
    return f"""
!
! void pvr_pack_sh4(const ClipVertex* in, pvr_vertex_packed_t* out, uint32_t n,
!                   const float* k)
!
! For each vertex writes the submit-ready PVR vertex - flags, screen x, y,
! 1/w, u, v, argb, oargb - to consecutive 32-byte slots of out (the
! clipping path works its clip position out again from the source).
! k = {{half width, half height, 127.5, -127.5}}. flags is
! PVR_CMD_VERTEX if the vertex is finite and in front of the near plane,
! else 0. Colours are assumed non-negative and clamped above at 1: with
! d = 127.5 (c - 1), trunc(|d| - d) = 255 (1 - min(c, 1)), and the packed
! words are built from those complements and inverted once at the end
! (which also makes the offset colour's alpha 0xFF).
!
! {PK_II} cycles per vertex; one register set (fr1-11 data, fr0 = 127.5,
! fr12 = -127.5, fr13/fr14 = half width/height, fr15 = 0). FPUL carries the
! seven float->int conversions one after another.
!
    .section .text._pvr_pack_sh4, "ax", %progbits
    .globl _pvr_pack_sh4
    .align 5
_pvr_pack_sh4:
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr15, @-r15
    mov     r4, r1              ! in
    mov     r5, r2
    add     #24, r2             ! out (see MC below)
    fmov.s  @r7+, fr13          ! half width
    fmov.s  @r7+, fr14          ! half height
    fmov.s  @r7+, fr0           ! 127.5
    fmov.s  @r7, fr12           ! -127.5
    fldi0   fr15
    mov.l   .pk_cmd, r3
    mov     r6, r7              ! n
    mov     #{ip}, r0
    cmp/hs  r0, r7
    bf      .pk_single
{L(pro)}
    add     #-{ip}, r7
    tst     r7, r7
    bt      .pk_epilogue
    .align 5
.pk_kernel:
{L(ker)}
.pk_epilogue:
{L(epi)}
    bra     .pk_done
    nop
.pk_single:
    tst     r7, r7
    bt      .pk_done
.pk_single_loop:
{L(single)}
    dt      r7
    bf      .pk_single_loop
.pk_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    rts
    mov.l   @r15+, r8
    .align 2
.pk_cmd:
    .long   0xe0000000          ! PVR_CMD_VERTEX
"""


# ---------------------------------------------------------------------------
# Directional light pass, in two kernels: one register set can't hold a
# vertex through both FSRRAs (a ~55-cycle chain), so dirA does the dot
# products and parks x, z and w in the scratch row's free slots 12-14, and
# dirB - short-lived values, so two register sets - does the rest.
# ---------------------------------------------------------------------------
DIRA_II = 19
DIRA_OPS = [
    # scratch row: A = (N.P/|P|, N) -> fv4, B = (1/|P|, P) -> fv0
    ("LA0", "LS", "fmov.s  @r1+, fr4", []),
    ("LA1", "LS", "fmov.s  @r1+, fr5", [("LA0", 1)]),
    ("LA2", "LS", "fmov.s  @r1+, fr6", [("LA1", 1)]),
    ("LA3", "LS", "fmov.s  @r1+, fr7", [("LA2", 1)]),
    ("LB0", "LS", "fmov.s  @r1+, fr0", [("LA3", 1)]),
    ("LB1", "LS", "fmov.s  @r1+, fr1", [("LB0", 1)]),
    ("LB2", "LS", "fmov.s  @r1+, fr2", [("LB1", 1)]),
    ("LB3", "LS", "fmov.s  @r1, fr3", [("LB2", 1)]),
    ("AL", "EX", "add     #36, r1", [("LB3", 1)]),
    # fv8 = (0, -L): fr7 = -N.L, fr3 = -P.L
    ("F1", "FE", "fipr    fv8, fv4", [("LA0", LOAD), ("LA1", LOAD), ("LA2", LOAD), ("LA3", LOAD)]),
    ("F2", "FE", "fipr    fv8, fv0", [("LB0", LOAD), ("LB1", LOAD), ("LB2", LOAD), ("LB3", LOAD)]),
    # x = c - P.L / |P|, a hair over 1 + L.V
    ("CX", "LS", "fmov    fr12, fr1", [("F2", 1)]),
    ("X", "FE", "fmac    fr0, fr3, fr1", [("F2", FIPR), ("CX", 1)]),
    # z' = N.P/|P| - N.L
    ("Z1", "FE", "fadd    fr7, fr4", [("F1", FIPR)]),
    # w = |N.L'| - N.L' = 2 max(N.L, 0)   (N.L' = -N.L)
    ("WC", "LS", "fmov    fr7, fr2", [("F1", FIPR), ("F2", 1)]),
    ("WA", "LS", "fabs    fr2", [("WC", 1)]),
    ("WS", "FE", "fsub    fr7, fr2", [("WA", 1)]),
    # slots 14, 13, 12 = x, z', w
    ("SX", "LS", "fmov.s  fr1, @-r2", [("X", FMUL_ST)]),
    ("SZ", "LS", "fmov.s  fr4, @-r2", [("SX", 1), ("Z1", FMUL_ST)]),
    ("SW", "LS", "fmov.s  fr2, @-r2", [("SZ", 1), ("WS", FMUL_ST)]),
    ("AS", "EX", "add     #76, r2", [("SW", 1)]),
]
DIRA_BUSY = {}
DIRA_REUSE = [
    ("LA0", ["Z1", "SZ"]), ("LA1", ["F1"]), ("LA2", ["F1"]), ("LA3", ["F1", "Z1", "WC", "WS"]),
    ("LB0", ["F2", "X"]), ("LB1", ["F2", "SX"]), ("LB2", ["F2", "SW"]),
    ("LB3", ["F2", "X"]), ("LA0", ["AL"]), ("SX", ["AS"]),
]

DIRB_II = 17
DIRB_OPS = [
    ("LW", "LS", "fmov.s  @{rl}+, {f0}", []),
    ("LZ", "LS", "fmov.s  @{rl}+, {f1}", [("LW", 1)]),
    ("LX", "LS", "fmov.s  @{rl}, {f2}", [("LZ", 1)]),
    ("AL", "EX", "add     #120, {rl}", [("LX", 1)]),
    # w is final already: store it, then keep w * scale
    ("WW", "LS", "fmov.s  {f0}, @{rs}", [("LW", LOAD)]),
    ("A4", "EX", "add     #4, {rs}", [("WW", 1)]),
    ("SW", "FE", "fmul    fr14, {f0}", [("LW", LOAD), ("WW", 1)]),
    # rh = 1/sqrt(x) = sqrt(2)/|L + V|; z = z' rh = -sqrt(2) N.H
    ("RH", "FE", "fsrra   {f2}", [("LX", LOAD)]),
    ("Z2", "FE", "fmul    {f2}, {f1}", [("RH", FSRRA), ("LZ", LOAD)]),
    # h = |z| - z = 2 sqrt(2) max(N.H, 0)
    ("HC", "LS", "fmov    {f1}, {f3}", [("Z2", FMUL)]),
    ("HA", "LS", "fabs    {f3}", [("HC", 1)]),
    ("HS", "FE", "fsub    {f1}, {f3}", [("HA", 1)]),
    # -d = c1 h - n; 1/d = 1/sqrt(d^2)
    ("DC", "LS", "fmov    {f3}, {f4}", [("HS", FMUL)]),
    ("DM", "FE", "fmul    fr12, {f4}", [("DC", 1)]),
    ("DA", "FE", "fadd    fr13, {f4}", [("DM", FMUL)]),
    ("DQ", "FE", "fmul    {f4}, {f4}", [("DA", FMUL)]),
    ("DR", "FE", "fsrra   {f4}", [("DQ", FMUL)]),
    # s = (w scale) h / d
    ("S1", "FE", "fmul    {f0}, {f3}", [("HS", FMUL), ("SW", FMUL), ("DC", 1)]),
    ("S2", "FE", "fmul    {f4}, {f3}", [("S1", FMUL), ("DR", FSRRA)]),
    ("WS", "LS", "fmov.s  {f3}, @{rs}", [("S2", FMUL_ST), ("A4", 1)]),
    ("AS", "EX", "add     #124, {rs}", [("WS", 1)]),
]
DIRB_BUSY = {"RH": 3, "DR": 3}
DIRB_REUSE = [
    ("LW", ["WW", "SW", "S1"]), ("LZ", ["Z2", "HC", "HS"]), ("LX", ["RH", "Z2"]),
    ("HC", ["S1", "S2", "WS", "DC"]), ("DC", ["DM", "DA", "DQ", "DR", "S2"]),
    ("LW", ["AL"]), ("WW", ["AS"]),
]
DIRB_SETS = {
    0: dict(rl="r1", rs="r2", f0="fr0", f1="fr1", f2="fr2", f3="fr3", f4="fr8"),
    1: dict(rl="r5", rs="r6", f0="fr4", f1="fr5", f2="fr6", f3="fr7", f4="fr9"),
}


def dirb_section(label):
    """dirB over the window (see DIRB_OPS), as the second half of a light
    kernel: expects rows in r8, weights in r9, n in r10 and dirB's constants
    (c1, -n, scale) at r11. Ends at .<label>_done."""
    sb = cached_solve("dirb", DIRB_OPS, DIRB_BUSY, DIRB_REUSE, DIRB_II, sets=DIRB_SETS[0])
    bpro, bker, bepi, bsingle, bip = gen_loop(
        DIRB_OPS, DIRB_SETS, sb, DIRB_II, label,
        single_subst=[("add     #120,", "add     #56,"), ("add     #124,", "add     #60,")])
    L = "\n".join
    return f"""    ! --- dirB
    fmov.s  @r11+, fr12         ! c1
    fmov.s  @r11+, fr13         ! -n
    fmov.s  @r11, fr14          ! scale
    mov     r8, r1
    add     #48, r1             ! a: slots 12.., row 0
    mov     r1, r5
    add     #64, r5             ! b: row 1
    mov     r9, r2              ! a: weights of row 0
    mov     r2, r6
    add     #64, r6             ! b: row 1
    mov     r10, r7
    shlr    r7                  ! pairs; T = n & 1
    movt    r3                  ! single-vertex tail count
    mov     #{bip}, r0
    cmp/hs  r0, r7
    bt      .{label}_pipelined
    mov     r10, r3             ! fewer pairs than the pipeline is deep:
    {('bra     .' + label + '_single'):<28}! all single
    nop
.{label}_pipelined:
{L(bpro)}
    add     #-{bip}, r7         ! kernel iterations = pairs - {bip}
    tst     r7, r7
    bt      .{label}_epilogue
    .align 5
.{label}_kernel:
{L(bker)}
.{label}_epilogue:
{L(bepi)}
.{label}_single:
    tst     r3, r3
    bt      .{label}_done
.{label}_single_loop:
{L(bsingle)}
    dt      r3
    bf      .{label}_single_loop
.{label}_done:
"""


LIGHT_EXIT = """    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    rts
    mov.l   @r15+, r8
"""


# ---------------------------------------------------------------------------
# Point light: pointA computes what dirA does - x, z' and w, parked in row
# slots 14, 13, 12 - for a light whose direction varies per vertex, with the
# attenuation folded into w; dirB then finishes exactly as for a directional
# light. XMTRX holds [1, P] -> [sqrt_eps, P - Lpos]: there aren't the
# registers to keep the light position as constants.
# ---------------------------------------------------------------------------
PTA_II = 38
PTA_OPS = [
    # scratch row: invV -> fr0, P -> fr9-11 (+ fr1-3) through r1 (at slot 4),
    # then A = (N.P/|P|, N) -> fv4 through r3, loaded as late as it can be
    ("LV", "LS", "fmov.s  @r1+, fr0", []),
    ("LP0", "LS", "fmov.s  @r1+, fr9", [("LV", 1)]),
    ("LP1", "LS", "fmov.s  @r1+, fr10", [("LP0", 1)]),
    ("LP2", "LS", "fmov.s  @r1, fr11", [("LP1", 1)]),
    ("AL", "EX", "add     #52, r1", [("LP2", 1)]),
    ("LA0", "LS", "fmov.s  @r3+, fr4", []),
    ("LA1", "LS", "fmov.s  @r3+, fr5", [("LA0", 1)]),
    ("LA2", "LS", "fmov.s  @r3+, fr6", [("LA1", 1)]),
    ("LA3", "LS", "fmov.s  @r3, fr7", [("LA2", 1)]),
    ("A3", "EX", "add     #52, r3", [("LA3", 1)]),
    ("F8", "LS", "fldi1   fr8", []),
    ("C1", "LS", "fmov    fr9, fr1", [("LP0", LOAD)]),
    ("C2", "LS", "fmov    fr10, fr2", [("LP1", LOAD)]),
    ("C3", "LS", "fmov    fr11, fr3", [("LP2", LOAD)]),
    # fv8 = (sqrt_eps, l'), l' = P - Lpos = -l
    ("TR", "FE", "ftrv    xmtrx, fv8",
     [("LP0", LOAD), ("LP1", LOAD), ("LP2", LOAD), ("F8", 1),
      ("C1", 1), ("C2", 1), ("C3", 1)]),
    # fr7 = N.l' (+ eps N.P/|P|), fr3 = P.l' (+ eps/|P|), fr11 = l'.l' + eps^2
    ("PN", "FE", "fipr    fv8, fv4",
     [("TR", FTRV), ("LA0", LOAD), ("LA1", LOAD), ("LA2", LOAD), ("LA3", LOAD)]),
    ("PP", "FE", "fipr    fv8, fv0", [("TR", FTRV), ("LV", LOAD), ("C1", 1), ("C2", 1), ("C3", 1)]),
    ("LL", "FE", "fipr    fv8, fv8", [("TR", FTRV), ("PN", 1), ("PP", 1)]),
    ("CL", "LS", "fmov    fr11, fr15", [("LL", FIPR)]),
    # fr11 = 1/|l|
    ("RL", "FE", "fsrra   fr11", [("LL", FIPR), ("CL", 1)]),
    # fr7 = -N.L, fr3 = -P.L (L the unit vector to the light), fr15 = |l|
    ("NL", "FE", "fmul    fr11, fr7", [("RL", FSRRA), ("PN", FIPR)]),
    ("PL", "FE", "fmul    fr11, fr3", [("RL", FSRRA), ("PP", FIPR)]),
    ("Q", "FE", "fmul    fr11, fr15", [("RL", FSRRA), ("CL", 1)]),
    # x = c - P.L / |P|, a hair over 1 + L.V
    ("CX", "LS", "fmov    fr12, fr1", [("PP", 1)]),
    ("X", "FE", "fmac    fr0, fr3, fr1", [("PL", FMUL), ("CX", 1), ("LV", LOAD)]),
    # z' = N.P/|P| - N.L
    ("Z", "FE", "fadd    fr7, fr4", [("NL", FMUL), ("PN", 1)]),
    # w = |N.L'| - N.L' = 2 max(N.L, 0)
    ("WC", "LS", "fmov    fr7, fr2", [("NL", FMUL), ("PP", 1)]),
    ("WA", "LS", "fabs    fr2", [("WC", 1)]),
    ("WS", "FE", "fsub    fr7, fr2", [("WA", 1), ("NL", FMUL)]),
    # a = 1/2 - |l| inv_range / 2 = att / 2; |a| + a = max(att, 0)
    ("T", "FE", "fmul    fr13, fr15", [("Q", FMUL)]),
    ("AH", "LS", "fmov    fr14, fr5", [("PN", 1)]),
    ("AT", "FE", "fsub    fr15, fr5", [("AH", 1), ("T", FMUL)]),
    ("AC", "LS", "fmov    fr5, fr6", [("AT", FMUL), ("PN", 1)]),
    ("AB", "LS", "fabs    fr6", [("AC", 1)]),
    ("AM", "FE", "fadd    fr5, fr6", [("AB", 1), ("AT", FMUL)]),
    # w = 2 max(N.L, 0) max(att, 0)
    ("WF", "FE", "fmul    fr6, fr2", [("AM", FMUL), ("WS", FMUL)]),
    # slots 14, 13, 12 = x, z', w
    ("SX", "LS", "fmov.s  fr1, @-r2", [("X", FMUL_ST)]),
    ("SZ", "LS", "fmov.s  fr4, @-r2", [("SX", 1), ("Z", FMUL_ST)]),
    ("SW", "LS", "fmov.s  fr2, @-r2", [("SZ", 1), ("WF", FMUL_ST)]),
    ("AS", "EX", "add     #76, r2", [("SW", 1)]),
]
PTA_BUSY = {}
PTA_REUSE = [
    ("LA0", ["PN", "Z", "SZ"]), ("LA1", ["PN", "AT", "AM"]),
    ("LA2", ["PN", "AB", "AM", "WF"]), ("LA3", ["PN", "NL", "Z", "WC", "WS"]),
    ("LV", ["PP", "X"]), ("C1", ["PP", "X", "SX"]),
    ("C2", ["PP", "WS", "WF", "SW"]), ("C3", ["PP", "PL", "X"]),
    ("F8", ["TR", "PN", "PP", "LL"]),
    ("LP0", ["C1", "TR", "PN", "PP", "LL"]), ("LP1", ["C2", "TR", "PN", "PP", "LL"]),
    ("LP2", ["C3", "TR", "PN", "PP", "LL", "CL", "RL", "NL", "PL", "Q"]),
    ("CL", ["Q", "T", "AT"]),
    ("LV", ["AL"]), ("LA0", ["A3"]), ("SX", ["AS"]),
]


def light_point():
    sa = cached_solve("pta", PTA_OPS, PTA_BUSY, PTA_REUSE, PTA_II, nsets=1)
    apro, aker, aepi, asingle, aip = gen_loop(PTA_OPS, {0: {}}, sa, PTA_II, "pta")
    L = "\n".join
    return f"""
!
! void pvr_light_point_sh4(float* rows, float* w, uint32_t n, const float* k)
!
! One point light over a window of scratch rows, as pvr_light_dir_sh4:
! writes the light's diffuse and specular weights, both DOUBLED, at w, w + 1
! and uses row slots 12-14 as scratch. Clobbers XMTRX. k:
!   k[0..15]  XMTRX, column-major: column 0 = (sqrt_eps, -Lpos) (Lpos the
!             eye-space light position), columns 1-3 = the identity's
!   k[16]     c = 1 + 4e-6
!   k[17]     inv_range / 2
!   k[18]     1/2
!   k[19..21] (n - 1) / (2 sqrt 2), -n, scale / (2 sqrt 2), as for dirB
! Branch-free, with the attenuation (1 - |l| inv_range, clamped at 0)
! folded into w; same maths as light_pass<true> in the C++.
!
! pointA ({PTA_II} cycles/vertex, one register set) leaves x, z' and w in
! slots 14, 13, 12 as dirA does, and dirB finishes.
!
    .section .text._pvr_light_point_sh4, "ax", %progbits
    .globl _pvr_light_point_sh4
    .align 5
_pvr_light_point_sh4:
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr15, @-r15
    mov     r4, r8              ! rows
    mov     r5, r9              ! w
    mov     r6, r10             ! n
    frchg                       ! XMTRX = k[0..15]
    fmov.s  @r7+, fr0
    fmov.s  @r7+, fr1
    fmov.s  @r7+, fr2
    fmov.s  @r7+, fr3
    fmov.s  @r7+, fr4
    fmov.s  @r7+, fr5
    fmov.s  @r7+, fr6
    fmov.s  @r7+, fr7
    fmov.s  @r7+, fr8
    fmov.s  @r7+, fr9
    fmov.s  @r7+, fr10
    fmov.s  @r7+, fr11
    fmov.s  @r7+, fr12
    fmov.s  @r7+, fr13
    fmov.s  @r7+, fr14
    fmov.s  @r7+, fr15
    frchg
    fmov.s  @r7+, fr12          ! c
    fmov.s  @r7+, fr13          ! inv_range / 2
    fmov.s  @r7+, fr14          ! 1/2
    mov     r7, r11             ! c1, -n, scale for dirB
    ! --- pointA
    mov     r8, r1
    add     #16, r1             ! 1/|P|, P
    mov     r8, r3              ! N.P/|P|, N
    mov     r8, r2
    add     #60, r2             ! slots 14..12, back to front
    mov     r10, r7
    mov     #{aip}, r0
    cmp/hs  r0, r7
    bf      .pta_single
{L(apro)}
    add     #-{aip}, r7
    tst     r7, r7
    bt      .pta_epilogue
    .align 5
.pta_kernel:
{L(aker)}
.pta_epilogue:
{L(aepi)}
    bra     .pta_done
    nop
.pta_single:
    tst     r7, r7
    bt      .pta_done
.pta_single_loop:
{L(asingle)}
    dt      r7
    bf      .pta_single_loop
.pta_done:
{dirb_section("ptb")}{LIGHT_EXIT}"""


def light_dir():
    sa = cached_solve("dira", DIRA_OPS, DIRA_BUSY, DIRA_REUSE, DIRA_II, nsets=1)
    apro, aker, aepi, asingle, aip = gen_loop(DIRA_OPS, {0: {}}, sa, DIRA_II, "dira")
    L = "\n".join
    return f"""
!
! void pvr_light_dir_sh4(float* rows, float* w, uint32_t n, const float* k)
!
! One directional light over a window of scratch rows (64 bytes each):
! reads (N.P/|P|, N) and (1/|P|, P) from each row and writes the light's
! diffuse and specular weights, both DOUBLED (the combine halves them), at
! w, w + 1 (w advancing a row per vertex); uses row slots 12-14 as scratch.
! k:
!   k[0..3] = (0, -L)          toward-light direction, negated
!   k[4]    = c = 1 + 4e-6     keeps 1 + L.V positive under rounding
!   k[5]    = (n - 1) / (2 sqrt 2)   (n: the Blinn-Phong exponent)
!   k[6]    = -n
!   k[7]    = scale / (2 sqrt 2)
! Branch-free: max(x, 0) is (x + |x|) / 2, with the factors of 2 and sqrt 2
! folded into k. Same maths as light_pass<false> in the C++.
!
! Two kernels, as one register set can't carry a vertex through both
! FSRRAs: dirA ({DIRA_II} cycles/vertex, one set) does the dot products and
! parks x, z' and w in slots 14, 13, 12; dirB ({DIRB_II} cycles/vertex, two
! sets: a fr0-3 + fr8, b fr4-7 + fr9) finishes.
!
    .section .text._pvr_light_dir_sh4, "ax", %progbits
    .globl _pvr_light_dir_sh4
    .align 5
_pvr_light_dir_sh4:
    mov.l   r8, @-r15
    mov.l   r9, @-r15
    mov.l   r10, @-r15
    mov.l   r11, @-r15
    fmov.s  fr12, @-r15
    fmov.s  fr13, @-r15
    fmov.s  fr14, @-r15
    fmov.s  fr15, @-r15
    mov     r4, r8              ! rows
    mov     r5, r9              ! w
    mov     r6, r10             ! n
    fmov.s  @r7+, fr8           ! (0, -L)
    fmov.s  @r7+, fr9
    fmov.s  @r7+, fr10
    fmov.s  @r7+, fr11
    fmov.s  @r7+, fr12          ! c
    mov     r7, r11             ! c1, -n, scale for dirB
    ! --- dirA
    mov     r8, r1
    mov     r8, r2
    add     #60, r2             ! slots 14..12, back to front
    mov     r10, r7
    mov     #{aip}, r0
    cmp/hs  r0, r7
    bf      .dira_single
{L(apro)}
    add     #-{aip}, r7
    tst     r7, r7
    bt      .dira_epilogue
    .align 5
.dira_kernel:
{L(aker)}
.dira_epilogue:
{L(aepi)}
    bra     .dira_done
    nop
.dira_single:
    tst     r7, r7
    bt      .dira_done
.dira_single_loop:
{L(asingle)}
    dt      r7
    bf      .dira_single_loop
.dira_done:
{dirb_section("dirb")}{LIGHT_EXIT}"""


HEADER = """!! \\file
!  \\brief   SH4 assembly for the PVR renderer's per-vertex lighting kernels.
!
!  GENERATED - edit tools/sh4_asm/lightgen.py and regenerate with
!      python tools/sh4_asm/lightgen.py > simulant/renderers/pvr/pvr_lighting_sh4.s
!  The loops are software pipelined from schedules found by
!  tools/sh4_asm/modsched.py and cached in tools/sh4_asm/*_ii<II>.json,
!  against the SH7091 timings measured at
!  https://ornio.nilware.io/sh4-sim/timings/ (tools/sh4_asm/sh4timing.py).
!  Each kernel pass also spends 2 cycles on its taken bf/s, which issues
!  alone. Comments give each op's cycle within the kernel iteration, the
!  vertex it belongs to relative to the iteration's first, and its name in
!  lightgen.py.
!
!  All are called with FPSCR.PR = 0 and FPSCR.SZ = 0 (single precision,
!  single moves), the -m4-single default, and preserve the callee-saved
!  registers.
!!
"""

if __name__ == "__main__":
    print(HEADER + geometry() + combine(2) + combine(1) + pass1() + pack() + light_dir()
          + light_point(), end="")
