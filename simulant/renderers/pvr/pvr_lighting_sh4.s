!! \file
!  \brief   SH4 assembly for the PVR renderer's per-vertex lighting kernels.
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
! Scheduling: one vertex starts every 23 cycles, alternating between two
! register sets (a: fr0-7, source r2/r3, out r1; b: fr8-15, r5/r9, r8) so
! each may stay live for 46. The FP pipe sets the pace: after an FTRV
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
    fmov.s  @r3+, fr4           !  0 v+0 L3
    fsts    fpul, fr7           !  1 v+0 FU
    fmov.s  @r3+, fr5           !  2 v+0 L4
    fmov.s  @r3, fr6            !  3 v+0 L5
    fmov.s  @r2+, fr0           !  4 v+0 L0
    fmov.s  @r2+, fr1           !  5 v+0 L1
    ftrv    xmtrx, fv4          !  6 v+0 TN
    fldi1   fr3                 !  7 v+0 F1
    add     r6, r3              !  7 v+0 A1
    fmov.s  @r2, fr2            !  8 v+0 L2
    ftrv    xmtrx, fv0          ! 11 v+0 TP
    movca.l r0, @r1             ! 12 v+0 MC2
    add     #-4, r1             ! 13 v+0 A4
    fipr    fv4, fv4            ! 15 v+0 NN
    fipr    fv0, fv0            ! 18 v+0 PP
    fsrra   fr7                 ! 19 v+0 RN
    fsrra   fr3                 ! 22 v+0 RV
    fmov.s  @r9+, fr12          ! 23 v+1 L3
    fsts    fpul, fr15          ! 24 v+1 FU
    fmov.s  @r9+, fr13          ! 25 v+1 L4
    add     r6, r2              ! 25 v+0 A0
    fmov.s  @r9, fr14           ! 26 v+1 L5
    fmul    fr7, fr4            ! 26 v+0 MX
    fmov.s  @r5+, fr8           ! 27 v+1 L0
    fmul    fr7, fr6            ! 27 v+0 MZ
    fmov.s  @r5+, fr9           ! 28 v+1 L1
    fmul    fr7, fr5            ! 28 v+0 MY
    fldi0   fr7                 ! 29 v+0 Z7
    ftrv    xmtrx, fv12         ! 29 v+1 TN
    fldi1   fr11                ! 30 v+1 F1
    add     r6, r9              ! 30 v+1 A1
    fmov.s  @r5, fr10           ! 31 v+1 L2
    movca.l r0, @r1             ! 32 v+0 MC
    fmov.s  fr2, @r1            ! 33 v+0 S2
    fipr    fv0, fv4            ! 33 v+0 NP
    fmov.s  fr1, @-r1           ! 34 v+0 S1
    ftrv    xmtrx, fv8          ! 34 v+1 TP
    movca.l r0, @r8             ! 35 v+1 MC2
    add     #-4, r8             ! 36 v+1 A4
    fmov.s  fr0, @-r1           ! 37 v+0 S0
    fmov.s  fr3, @-r1           ! 38 v+0 SV
    fipr    fv12, fv12          ! 38 v+1 NN
    fmov.s  fr6, @-r1           ! 39 v+0 SZ
    fmov.s  fr5, @-r1           ! 40 v+0 SY
    fmul    fr3, fr7            ! 40 v+0 M0
    fmov.s  fr4, @-r1           ! 41 v+0 SX
    fipr    fv8, fv8            ! 41 v+1 PP
    fmov.s  fr7, @-r1           ! 42 v+0 SD
    fsrra   fr15                ! 42 v+1 RN
    add     r4, r1              ! 43 v+0 AO
    fsrra   fr11                ! 45 v+1 RV
    dt      r7                  ! kernel iterations = pairs - 1
    bt      .geo_epilogue
    .align 5
.geo_kernel:
    fmov.s  @r3+, fr4           !  0 v+0 L3
    dt      r7                  !  0 loop count
    fsts    fpul, fr7           !  1 v+0 FU
    fmov.s  @r3+, fr5           !  2 v+0 L4
    add     r6, r5              !  2 v-1 A0
    fmov.s  @r3, fr6            !  3 v+0 L5
    fmul    fr15, fr12          !  3 v-1 MX
    fmov.s  @r2+, fr0           !  4 v+0 L0
    fmul    fr15, fr14          !  4 v-1 MZ
    fmov.s  @r2+, fr1           !  5 v+0 L1
    fmul    fr15, fr13          !  5 v-1 MY
    fldi0   fr15                !  6 v-1 Z7
    ftrv    xmtrx, fv4          !  6 v+0 TN
    fldi1   fr3                 !  7 v+0 F1
    add     r6, r3              !  7 v+0 A1
    fmov.s  @r2, fr2            !  8 v+0 L2
    movca.l r0, @r8             !  9 v-1 MC
    fmov.s  fr10, @r8           ! 10 v-1 S2
    fipr    fv8, fv12           ! 10 v-1 NP
    fmov.s  fr9, @-r8           ! 11 v-1 S1
    ftrv    xmtrx, fv0          ! 11 v+0 TP
    movca.l r0, @r1             ! 12 v+0 MC2
    add     #-4, r1             ! 13 v+0 A4
    fmov.s  fr8, @-r8           ! 14 v-1 S0
    fmov.s  fr11, @-r8          ! 15 v-1 SV
    fipr    fv4, fv4            ! 15 v+0 NN
    fmov.s  fr14, @-r8          ! 16 v-1 SZ
    fmov.s  fr13, @-r8          ! 17 v-1 SY
    fmul    fr11, fr15          ! 17 v-1 M0
    fmov.s  fr12, @-r8          ! 18 v-1 SX
    fipr    fv0, fv0            ! 18 v+0 PP
    fmov.s  fr15, @-r8          ! 19 v-1 SD
    fsrra   fr7                 ! 19 v+0 RN
    add     r4, r8              ! 20 v-1 AO
    fsrra   fr3                 ! 22 v+0 RV
    fmov.s  @r9+, fr12          ! 23 v+1 L3
    fsts    fpul, fr15          ! 24 v+1 FU
    fmov.s  @r9+, fr13          ! 25 v+1 L4
    add     r6, r2              ! 25 v+0 A0
    fmov.s  @r9, fr14           ! 26 v+1 L5
    fmul    fr7, fr4            ! 26 v+0 MX
    fmov.s  @r5+, fr8           ! 27 v+1 L0
    fmul    fr7, fr6            ! 27 v+0 MZ
    fmov.s  @r5+, fr9           ! 28 v+1 L1
    fmul    fr7, fr5            ! 28 v+0 MY
    fldi0   fr7                 ! 29 v+0 Z7
    ftrv    xmtrx, fv12         ! 29 v+1 TN
    fldi1   fr11                ! 30 v+1 F1
    add     r6, r9              ! 30 v+1 A1
    fmov.s  @r5, fr10           ! 31 v+1 L2
    movca.l r0, @r1             ! 32 v+0 MC
    fmov.s  fr2, @r1            ! 33 v+0 S2
    fipr    fv0, fv4            ! 33 v+0 NP
    fmov.s  fr1, @-r1           ! 34 v+0 S1
    ftrv    xmtrx, fv8          ! 34 v+1 TP
    movca.l r0, @r8             ! 35 v+1 MC2
    add     #-4, r8             ! 36 v+1 A4
    fmov.s  fr0, @-r1           ! 37 v+0 S0
    fmov.s  fr3, @-r1           ! 38 v+0 SV
    fipr    fv12, fv12          ! 38 v+1 NN
    fmov.s  fr6, @-r1           ! 39 v+0 SZ
    fmov.s  fr5, @-r1           ! 40 v+0 SY
    fmul    fr3, fr7            ! 40 v+0 M0
    fmov.s  fr4, @-r1           ! 41 v+0 SX
    fipr    fv8, fv8            ! 41 v+1 PP
    fmov.s  fr7, @-r1           ! 42 v+0 SD
    fsrra   fr15                ! 42 v+1 RN
    add     r4, r1              ! 43 v+0 AO
    bf/s    .geo_kernel
    fsrra   fr11                ! 45 v+1 RV
.geo_epilogue:
    add     r6, r5              !  2 v-1 A0
    fmul    fr15, fr12          !  3 v-1 MX
    fmul    fr15, fr14          !  4 v-1 MZ
    fmul    fr15, fr13          !  5 v-1 MY
    fldi0   fr15                !  6 v-1 Z7
    movca.l r0, @r8             !  9 v-1 MC
    fmov.s  fr10, @r8           ! 10 v-1 S2
    fipr    fv8, fv12           ! 10 v-1 NP
    fmov.s  fr9, @-r8           ! 11 v-1 S1
    fmov.s  fr8, @-r8           ! 14 v-1 S0
    fmov.s  fr11, @-r8          ! 15 v-1 SV
    fmov.s  fr14, @-r8          ! 16 v-1 SZ
    fmov.s  fr13, @-r8          ! 17 v-1 SY
    fmul    fr11, fr15          ! 17 v-1 M0
    fmov.s  fr12, @-r8          ! 18 v-1 SX
    fmov.s  fr15, @-r8          ! 19 v-1 SD
    add     r4, r8              ! 20 v-1 AO
.geo_single:
    tst     r10, r10
    bt      .geo_done
    fmov.s  @r3+, fr4           !  0 L3
    fsts    fpul, fr7           !  1 FU
    fmov.s  @r3+, fr5           !  2 L4
    fmov.s  @r3, fr6            !  3 L5
    fmov.s  @r2+, fr0           !  4 L0
    fmov.s  @r2+, fr1           !  5 L1
    ftrv    xmtrx, fv4          !  6 TN
    fldi1   fr3                 !  7 F1
    add     r11, r3             !  7 A1
    fmov.s  @r2, fr2            !  8 L2
    ftrv    xmtrx, fv0          ! 11 TP
    movca.l r0, @r1             ! 12 MC2
    add     #-4, r1             ! 13 A4
    fipr    fv4, fv4            ! 15 NN
    fipr    fv0, fv0            ! 18 PP
    fsrra   fr7                 ! 19 RN
    fsrra   fr3                 ! 22 RV
    add     r11, r2             ! 25 A0
    fmul    fr7, fr4            ! 26 MX
    fmul    fr7, fr6            ! 27 MZ
    fmul    fr7, fr5            ! 28 MY
    fldi0   fr7                 ! 29 Z7
    movca.l r0, @r1             ! 32 MC
    fmov.s  fr2, @r1            ! 33 S2
    fipr    fv0, fv4            ! 33 NP
    fmov.s  fr1, @-r1           ! 34 S1
    fmov.s  fr0, @-r1           ! 37 S0
    fmov.s  fr3, @-r1           ! 38 SV
    fmov.s  fr6, @-r1           ! 39 SZ
    fmov.s  fr5, @-r1           ! 40 SY
    fmul    fr3, fr7            ! 40 M0
    fmov.s  fr4, @-r1           ! 41 SX
    fmov.s  fr7, @-r1           ! 42 SD
    add     #96, r1             ! 43 AO
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

!
! void pvr_light_combine3_sh4(float* w, uint32_t n)
!
! r4 : the first scratch row's light weights: wd0, ws0, wd1, ws1 (wd2, ws2)
!      at +0..+12 (+20)
! r5 : vertex count
!
! On entry XMTRX holds column 0 = light 0's colour, column 1 = light 1's,
! column 2 = light 2's (0 for fewer lights), column 3 = (ambient, 1). Then
! with D = (wd0, wd1, wd2, 1) and S = (ws0, ws1, ws2, 0)
!   FTRV(D) = (ambient + sum(wd * colour), 1)
!   FTRV(S) = (sum(ws * colour), 0)
! which replace the weights in each row: D's rgb at +0..+8 and S's at
! +16..+24 (64-byte row stride). Row 3 of the matrix keeps the w lanes at 1
! and 0, so after the setup below they don't need rewriting per vertex; with
! fewer than three lights lane 2 is never loaded and meets a zero column.
!
! 12 cycles per vertex (the two FTRVs hold FE for 8), pipelined as in the
! geometry pass (a: fr0-7, row pointer r1; b: fr8-15, r5).
!
    .section .text._pvr_light_combine3_sh4, "ax", %progbits
    .globl _pvr_light_combine3_sh4
    .align 5
_pvr_light_combine3_sh4:
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
    fldi0   fr14
    shlr    r7                  ! pairs; T = n & 1
    movt    r6                  ! single-vertex tail count (all of n if n < 2)
    tst     r7, r7
    bt      .comb3_single
    fmov.s  @r1+, fr0           !  0 v+0 LD0
    fmov.s  @r1+, fr4           !  1 v+0 LS0
    fmov.s  @r1+, fr1           !  2 v+0 LD1
    fmov.s  @r1+, fr5           !  3 v+0 LS1
    fmov.s  @r1+, fr2           !  7 v+0 LD2
    ftrv    xmtrx, fv0          ! 10 v+0 TD
    fmov.s  @r1, fr6            ! 11 v+0 LS2
    fmov.s  @r5+, fr8           ! 12 v+1 LD0
    fmov.s  @r5+, fr12          ! 13 v+1 LS0
    fmov.s  @r5+, fr9           ! 14 v+1 LD1
    ftrv    xmtrx, fv4          ! 14 v+0 TS
    fmov.s  @r5+, fr13          ! 15 v+1 LS1
    add     #-8, r1             ! 15 v+0 AB8
    fmov.s  fr2, @-r1           ! 16 v+0 DB
    fmov.s  fr1, @-r1           ! 17 v+0 DG
    fmov.s  fr0, @-r1           ! 18 v+0 DR
    fmov.s  @r5+, fr10          ! 19 v+1 LD2
    add     #28, r1             ! 19 v+0 A28
    fmov.s  fr6, @-r1           ! 20 v+0 SB
    fmov.s  fr5, @-r1           ! 21 v+0 SG
    fmov.s  fr4, @-r1           ! 22 v+0 SR
    ftrv    xmtrx, fv8          ! 22 v+1 TD
    fmov.s  @r5, fr14           ! 23 v+1 LS2
    add     #112, r1            ! 23 v+0 AW
    dt      r7                  ! kernel iterations = pairs - 1
    bt      .comb3_epilogue
    .align 5
.comb3_kernel:
    fmov.s  @r1+, fr0           !  0 v+0 LD0
    dt      r7                  !  0 loop count
    fmov.s  @r1+, fr4           !  1 v+0 LS0
    fmov.s  @r1+, fr1           !  2 v+0 LD1
    ftrv    xmtrx, fv12         !  2 v-1 TS
    fmov.s  @r1+, fr5           !  3 v+0 LS1
    add     #-8, r5             !  3 v-1 AB8
    fmov.s  fr10, @-r5          !  4 v-1 DB
    fmov.s  fr9, @-r5           !  5 v-1 DG
    fmov.s  fr8, @-r5           !  6 v-1 DR
    fmov.s  @r1+, fr2           !  7 v+0 LD2
    add     #28, r5             !  7 v-1 A28
    fmov.s  fr14, @-r5          !  8 v-1 SB
    fmov.s  fr13, @-r5          !  9 v-1 SG
    fmov.s  fr12, @-r5          ! 10 v-1 SR
    ftrv    xmtrx, fv0          ! 10 v+0 TD
    fmov.s  @r1, fr6            ! 11 v+0 LS2
    add     #112, r5            ! 11 v-1 AW
    fmov.s  @r5+, fr8           ! 12 v+1 LD0
    fmov.s  @r5+, fr12          ! 13 v+1 LS0
    fmov.s  @r5+, fr9           ! 14 v+1 LD1
    ftrv    xmtrx, fv4          ! 14 v+0 TS
    fmov.s  @r5+, fr13          ! 15 v+1 LS1
    add     #-8, r1             ! 15 v+0 AB8
    fmov.s  fr2, @-r1           ! 16 v+0 DB
    fmov.s  fr1, @-r1           ! 17 v+0 DG
    fmov.s  fr0, @-r1           ! 18 v+0 DR
    fmov.s  @r5+, fr10          ! 19 v+1 LD2
    add     #28, r1             ! 19 v+0 A28
    fmov.s  fr6, @-r1           ! 20 v+0 SB
    fmov.s  fr5, @-r1           ! 21 v+0 SG
    fmov.s  fr4, @-r1           ! 22 v+0 SR
    ftrv    xmtrx, fv8          ! 22 v+1 TD
    fmov.s  @r5, fr14           ! 23 v+1 LS2
    bf/s    .comb3_kernel
    add     #112, r1            ! 23 v+0 AW
.comb3_epilogue:
    ftrv    xmtrx, fv12         !  2 v-1 TS
    add     #-8, r5             !  3 v-1 AB8
    fmov.s  fr10, @-r5          !  4 v-1 DB
    fmov.s  fr9, @-r5           !  5 v-1 DG
    fmov.s  fr8, @-r5           !  6 v-1 DR
    add     #28, r5             !  7 v-1 A28
    fmov.s  fr14, @-r5          !  8 v-1 SB
    fmov.s  fr13, @-r5          !  9 v-1 SG
    fmov.s  fr12, @-r5          ! 10 v-1 SR
    add     #112, r5            ! 11 v-1 AW
.comb3_single:
    tst     r6, r6
    bt      .comb3_done
    fmov.s  @r1+, fr0           !  0 LD0
    fmov.s  @r1+, fr4           !  1 LS0
    fmov.s  @r1+, fr1           !  2 LD1
    fmov.s  @r1+, fr5           !  3 LS1
    fmov.s  @r1+, fr2           !  7 LD2
    ftrv    xmtrx, fv0          ! 10 TD
    fmov.s  @r1, fr6            ! 11 LS2
    ftrv    xmtrx, fv4          ! 14 TS
    add     #-8, r1             ! 15 AB8
    fmov.s  fr2, @-r1           ! 16 DB
    fmov.s  fr1, @-r1           ! 17 DG
    fmov.s  fr0, @-r1           ! 18 DR
    add     #28, r1             ! 19 A28
    fmov.s  fr6, @-r1           ! 20 SB
    fmov.s  fr5, @-r1           ! 21 SG
    fmov.s  fr4, @-r1           ! 22 SR
    add     #48, r1             ! 23 AW
.comb3_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    rts
    fmov.s  @r15+, fr12

!
! void pvr_light_combine2_sh4(float* w, uint32_t n)
!
! r4 : the first scratch row's light weights: wd0, ws0, wd1, ws1 (wd2, ws2)
!      at +0..+12 (+20)
! r5 : vertex count
!
! On entry XMTRX holds column 0 = light 0's colour, column 1 = light 1's,
! column 2 = light 2's (0 for fewer lights), column 3 = (ambient, 1). Then
! with D = (wd0, wd1, wd2, 1) and S = (ws0, ws1, ws2, 0)
!   FTRV(D) = (ambient + sum(wd * colour), 1)
!   FTRV(S) = (sum(ws * colour), 0)
! which replace the weights in each row: D's rgb at +0..+8 and S's at
! +16..+24 (64-byte row stride). Row 3 of the matrix keeps the w lanes at 1
! and 0, so after the setup below they don't need rewriting per vertex; with
! fewer than three lights lane 2 is never loaded and meets a zero column.
!
! 11 cycles per vertex (the two FTRVs hold FE for 8), pipelined as in the
! geometry pass (a: fr0-7, row pointer r1; b: fr8-15, r5).
!
    .section .text._pvr_light_combine2_sh4, "ax", %progbits
    .globl _pvr_light_combine2_sh4
    .align 5
_pvr_light_combine2_sh4:
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
    fldi0   fr14
    shlr    r7                  ! pairs; T = n & 1
    movt    r6                  ! single-vertex tail count (all of n if n < 2)
    tst     r7, r7
    bt      .comb2_single
    fmov.s  @r1+, fr0           !  0 v+0 LD0
    fmov.s  @r1+, fr4           !  1 v+0 LS0
    fmov.s  @r1+, fr1           !  2 v+0 LD1
    ftrv    xmtrx, fv0          !  5 v+0 TD
    fmov.s  @r1, fr5            !  9 v+0 LS1
    fmov.s  fr2, @-r1           ! 10 v+0 DB
    fmov.s  @r5+, fr8           ! 11 v+1 LD0
    fmov.s  @r5+, fr12          ! 12 v+1 LS0
    ftrv    xmtrx, fv4          ! 12 v+0 TS
    fmov.s  @r5+, fr9           ! 13 v+1 LD1
    fmov.s  fr1, @-r1           ! 14 v+0 DG
    fmov.s  fr0, @-r1           ! 15 v+0 DR
    add     #28, r1             ! 16 v+0 A28
    ftrv    xmtrx, fv8          ! 16 v+1 TD
    fmov.s  fr6, @-r1           ! 17 v+0 SB
    fmov.s  fr5, @-r1           ! 18 v+0 SG
    fmov.s  fr4, @-r1           ! 19 v+0 SR
    fmov.s  @r5, fr13           ! 20 v+1 LS1
    add     #112, r1            ! 20 v+0 AW
    fmov.s  fr10, @-r5          ! 21 v+1 DB
    dt      r7                  ! kernel iterations = pairs - 1
    bt      .comb2_epilogue
    .align 5
.comb2_kernel:
    fmov.s  @r1+, fr0           !  0 v+0 LD0
    dt      r7                  !  0 loop count
    fmov.s  @r1+, fr4           !  1 v+0 LS0
    ftrv    xmtrx, fv12         !  1 v-1 TS
    fmov.s  @r1+, fr1           !  2 v+0 LD1
    fmov.s  fr9, @-r5           !  3 v-1 DG
    fmov.s  fr8, @-r5           !  4 v-1 DR
    add     #28, r5             !  5 v-1 A28
    ftrv    xmtrx, fv0          !  5 v+0 TD
    fmov.s  fr14, @-r5          !  6 v-1 SB
    fmov.s  fr13, @-r5          !  7 v-1 SG
    fmov.s  fr12, @-r5          !  8 v-1 SR
    fmov.s  @r1, fr5            !  9 v+0 LS1
    add     #112, r5            !  9 v-1 AW
    fmov.s  fr2, @-r1           ! 10 v+0 DB
    fmov.s  @r5+, fr8           ! 11 v+1 LD0
    fmov.s  @r5+, fr12          ! 12 v+1 LS0
    ftrv    xmtrx, fv4          ! 12 v+0 TS
    fmov.s  @r5+, fr9           ! 13 v+1 LD1
    fmov.s  fr1, @-r1           ! 14 v+0 DG
    fmov.s  fr0, @-r1           ! 15 v+0 DR
    add     #28, r1             ! 16 v+0 A28
    ftrv    xmtrx, fv8          ! 16 v+1 TD
    fmov.s  fr6, @-r1           ! 17 v+0 SB
    fmov.s  fr5, @-r1           ! 18 v+0 SG
    fmov.s  fr4, @-r1           ! 19 v+0 SR
    fmov.s  @r5, fr13           ! 20 v+1 LS1
    add     #112, r1            ! 20 v+0 AW
    bf/s    .comb2_kernel
    fmov.s  fr10, @-r5          ! 21 v+1 DB
.comb2_epilogue:
    ftrv    xmtrx, fv12         !  1 v-1 TS
    fmov.s  fr9, @-r5           !  3 v-1 DG
    fmov.s  fr8, @-r5           !  4 v-1 DR
    add     #28, r5             !  5 v-1 A28
    fmov.s  fr14, @-r5          !  6 v-1 SB
    fmov.s  fr13, @-r5          !  7 v-1 SG
    fmov.s  fr12, @-r5          !  8 v-1 SR
    add     #112, r5            !  9 v-1 AW
.comb2_single:
    tst     r6, r6
    bt      .comb2_done
    fmov.s  @r1+, fr0           !  0 LD0
    fmov.s  @r1+, fr4           !  1 LS0
    fmov.s  @r1+, fr1           !  2 LD1
    ftrv    xmtrx, fv0          !  5 TD
    fmov.s  @r1, fr5            !  9 LS1
    fmov.s  fr2, @-r1           ! 10 DB
    ftrv    xmtrx, fv4          ! 12 TS
    fmov.s  fr1, @-r1           ! 14 DG
    fmov.s  fr0, @-r1           ! 15 DR
    add     #28, r1             ! 16 A28
    fmov.s  fr6, @-r1           ! 17 SB
    fmov.s  fr5, @-r1           ! 18 SG
    fmov.s  fr4, @-r1           ! 19 SR
    add     #48, r1             ! 20 AW
.comb2_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    rts
    fmov.s  @r15+, fr12

!
! void pvr_light_combine1_sh4(float* w, uint32_t n)
!
! r4 : the first scratch row's light weights: wd0, ws0, wd1, ws1 (wd2, ws2)
!      at +0..+12 (+20)
! r5 : vertex count
!
! On entry XMTRX holds column 0 = light 0's colour, column 1 = light 1's,
! column 2 = light 2's (0 for fewer lights), column 3 = (ambient, 1). Then
! with D = (wd0, wd1, wd2, 1) and S = (ws0, ws1, ws2, 0)
!   FTRV(D) = (ambient + sum(wd * colour), 1)
!   FTRV(S) = (sum(ws * colour), 0)
! which replace the weights in each row: D's rgb at +0..+8 and S's at
! +16..+24 (64-byte row stride). Row 3 of the matrix keeps the w lanes at 1
! and 0, so after the setup below they don't need rewriting per vertex; with
! fewer than three lights lane 2 is never loaded and meets a zero column.
!
! 9 cycles per vertex (the two FTRVs hold FE for 8), pipelined as in the
! geometry pass (a: fr0-7, row pointer r1; b: fr8-15, r5).
!
    .section .text._pvr_light_combine1_sh4, "ax", %progbits
    .globl _pvr_light_combine1_sh4
    .align 5
_pvr_light_combine1_sh4:
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
    fldi0   fr14
    fldi0   fr1                 ! light 1's lanes: never loaded, times a
    fldi0   fr5                 ! zero column
    fldi0   fr9
    fldi0   fr13
    shlr    r7                  ! pairs; T = n & 1
    movt    r6                  ! single-vertex tail count (all of n if n < 2)
    tst     r7, r7
    bt      .comb1_single
    fmov.s  @r1+, fr0           !  0 v+0 LD0
    fmov.s  @r1, fr4            !  4 v+0 LS0
    ftrv    xmtrx, fv0          !  5 v+0 TD
    add     #8, r1              !  7 v+0 A8
    fmov.s  @r5+, fr8           !  9 v+1 LD0
    ftrv    xmtrx, fv4          !  9 v+0 TS
    fmov.s  fr2, @-r1           ! 10 v+0 DB
    fmov.s  fr1, @-r1           ! 11 v+0 DG
    fmov.s  fr0, @-r1           ! 12 v+0 DR
    fmov.s  @r5, fr12           ! 13 v+1 LS0
    add     #28, r1             ! 13 v+0 A28
    fmov.s  fr6, @-r1           ! 14 v+0 SB
    ftrv    xmtrx, fv8          ! 14 v+1 TD
    fmov.s  fr5, @-r1           ! 15 v+0 SG
    fmov.s  fr4, @-r1           ! 16 v+0 SR
    add     #8, r5              ! 16 v+1 A8
    add     #112, r1            ! 17 v+0 AW
    dt      r7                  ! kernel iterations = pairs - 1
    bt      .comb1_epilogue
    .align 5
.comb1_kernel:
    fmov.s  @r1+, fr0           !  0 v+0 LD0
    ftrv    xmtrx, fv12         !  0 v-1 TS
    fmov.s  fr10, @-r5          !  1 v-1 DB
    dt      r7                  !  1 loop count
    fmov.s  fr9, @-r5           !  2 v-1 DG
    fmov.s  fr8, @-r5           !  3 v-1 DR
    fmov.s  @r1, fr4            !  4 v+0 LS0
    add     #28, r5             !  4 v-1 A28
    fmov.s  fr14, @-r5          !  5 v-1 SB
    ftrv    xmtrx, fv0          !  5 v+0 TD
    fmov.s  fr13, @-r5          !  6 v-1 SG
    fmov.s  fr12, @-r5          !  7 v-1 SR
    add     #8, r1              !  7 v+0 A8
    add     #112, r5            !  8 v-1 AW
    fmov.s  @r5+, fr8           !  9 v+1 LD0
    ftrv    xmtrx, fv4          !  9 v+0 TS
    fmov.s  fr2, @-r1           ! 10 v+0 DB
    fmov.s  fr1, @-r1           ! 11 v+0 DG
    fmov.s  fr0, @-r1           ! 12 v+0 DR
    fmov.s  @r5, fr12           ! 13 v+1 LS0
    add     #28, r1             ! 13 v+0 A28
    fmov.s  fr6, @-r1           ! 14 v+0 SB
    ftrv    xmtrx, fv8          ! 14 v+1 TD
    fmov.s  fr5, @-r1           ! 15 v+0 SG
    fmov.s  fr4, @-r1           ! 16 v+0 SR
    add     #8, r5              ! 16 v+1 A8
    bf/s    .comb1_kernel
    add     #112, r1            ! 17 v+0 AW
.comb1_epilogue:
    ftrv    xmtrx, fv12         !  0 v-1 TS
    fmov.s  fr10, @-r5          !  1 v-1 DB
    fmov.s  fr9, @-r5           !  2 v-1 DG
    fmov.s  fr8, @-r5           !  3 v-1 DR
    add     #28, r5             !  4 v-1 A28
    fmov.s  fr14, @-r5          !  5 v-1 SB
    fmov.s  fr13, @-r5          !  6 v-1 SG
    fmov.s  fr12, @-r5          !  7 v-1 SR
    add     #112, r5            !  8 v-1 AW
.comb1_single:
    tst     r6, r6
    bt      .comb1_done
    fmov.s  @r1+, fr0           !  0 LD0
    fmov.s  @r1, fr4            !  4 LS0
    ftrv    xmtrx, fv0          !  5 TD
    add     #8, r1              !  7 A8
    ftrv    xmtrx, fv4          !  9 TS
    fmov.s  fr2, @-r1           ! 10 DB
    fmov.s  fr1, @-r1           ! 11 DG
    fmov.s  fr0, @-r1           ! 12 DR
    add     #28, r1             ! 13 A28
    fmov.s  fr6, @-r1           ! 14 SB
    fmov.s  fr5, @-r1           ! 15 SG
    fmov.s  fr4, @-r1           ! 16 SR
    add     #48, r1             ! 17 AW
.comb1_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    rts
    fmov.s  @r15+, fr12

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
! 32 cycles per vertex, bound by the LS pipe (32 loads, stores, fldi
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
    mov     #1, r0
    cmp/hs  r0, r7
    bf      .p1_single          ! n < 1: too short to pipeline
    fmov.s  @r5+, fr7           !  0 v+0 LDR
    fmov.s  @r5+, fr8           !  1 v+0 LDG
    fmov.s  @r5+, fr9           !  2 v+0 LDB
    fldi1   fr3                 !  3 v+0 F1
    add     #4, r5              !  3 v+0 AL4
    fmov.s  @r5+, fr4           !  6 v+0 LSR
    fmov.s  @r5+, fr5           !  7 v+0 LSG
    fmov.s  @r5, fr6            !  8 v+0 LSB
    movca.l r0, @r6             !  9 v+0 M1
    add     r12, r5             !  9 v+0 ALS
    fmov.s  fr6, @-r6           ! 10 v+0 WSB
    fmov.s  fr5, @-r6           ! 11 v+0 WSG
    fmov.s  fr4, @-r6           ! 12 v+0 WSR
    fmov.s  @r3+, fr4           ! 13 v+0 LCR
    fmov.s  @r3+, fr5           ! 14 v+0 LCG
    fmov.s  @r3+, fr6           ! 15 v+0 LCB
    fmul    fr12, fr4           ! 15 v+0 MR1
    fmov.s  @r3, fr10           ! 16 v+0 LCA
    fmul    fr13, fr5           ! 16 v+0 MG1
    fmov.s  @r1+, fr0           ! 17 v+0 L0
    fmul    fr14, fr6           ! 17 v+0 MB1
    fmov.s  @r1+, fr1           ! 18 v+0 L1
    fmul    fr15, fr10          ! 18 v+0 MA
    fmov.s  @r1, fr2            ! 19 v+0 L2
    fmul    fr7, fr4            ! 19 v+0 MR2
    fmov.s  fr10, @-r6          ! 20 v+0 WA
    fmul    fr9, fr6            ! 20 v+0 MB2
    fmov.s  @r2+, fr7           ! 21 v+0 LU
    fmul    fr8, fr5            ! 21 v+0 MG2
    fmov.s  fr6, @-r6           ! 22 v+0 WB
    ftrv    xmtrx, fv0          ! 22 v+0 TR
    fmov.s  @r2, fr8            ! 23 v+0 LV
    add     #-4, r6             ! 23 v+0 D4
    movca.l r0, @r6             ! 24 v+0 M0
    add     r10, r3             ! 24 v+0 AC
    fmov.s  fr5, @r6            ! 25 v+0 WG
    fmov.s  fr4, @-r6           ! 26 v+0 WR
    fmov.s  fr8, @-r6           ! 27 v+0 WV
    add     r8, r1              ! 27 v+0 AP
    fmov.s  fr7, @-r6           ! 28 v+0 WU
    fmov.s  fr3, @-r6           ! 29 v+0 WW
    fmov.s  fr2, @-r6           ! 30 v+0 WZ
    fipr    fv0, fv0            ! 30 v+0 VF
    fmov.s  fr1, @-r6           ! 31 v+0 WY
    add     r9, r2              ! 31 v+0 AU
    add     #-1, r7          ! kernel iterations = n - 1
    tst     r7, r7
    bt      .p1_epilogue
    .align 5
.p1_kernel:
    fmov.s  @r5+, fr7           !  0 v+0 LDR
    fmov.s  @r5+, fr8           !  1 v+0 LDG
    fmov.s  @r5+, fr9           !  2 v+0 LDB
    fcmp/gt fr3, fr11           !  2 v-1 FC
    fldi1   fr3                 !  3 v+0 F1
    add     #4, r5              !  3 v+0 AL4
    fmov.s  fr0, @-r6           !  4 v-1 WX
    movt    r0                  !  4 v-1 MT
    mov.l   r0, @(52, r6)       !  5 v-1 OK
    add     r0, r4              !  5 v-1 AV
    fmov.s  @r5+, fr4           !  6 v+0 LSR
    add     #116, r6            !  6 v-1 AO
    fmov.s  @r5+, fr5           !  7 v+0 LSG
    dt      r7                  !  7 loop count
    fmov.s  @r5, fr6            !  8 v+0 LSB
    movca.l r0, @r6             !  9 v+0 M1
    add     r12, r5             !  9 v+0 ALS
    fmov.s  fr6, @-r6           ! 10 v+0 WSB
    fmov.s  fr5, @-r6           ! 11 v+0 WSG
    fmov.s  fr4, @-r6           ! 12 v+0 WSR
    fmov.s  @r3+, fr4           ! 13 v+0 LCR
    fmov.s  @r3+, fr5           ! 14 v+0 LCG
    fmov.s  @r3+, fr6           ! 15 v+0 LCB
    fmul    fr12, fr4           ! 15 v+0 MR1
    fmov.s  @r3, fr10           ! 16 v+0 LCA
    fmul    fr13, fr5           ! 16 v+0 MG1
    fmov.s  @r1+, fr0           ! 17 v+0 L0
    fmul    fr14, fr6           ! 17 v+0 MB1
    fmov.s  @r1+, fr1           ! 18 v+0 L1
    fmul    fr15, fr10          ! 18 v+0 MA
    fmov.s  @r1, fr2            ! 19 v+0 L2
    fmul    fr7, fr4            ! 19 v+0 MR2
    fmov.s  fr10, @-r6          ! 20 v+0 WA
    fmul    fr9, fr6            ! 20 v+0 MB2
    fmov.s  @r2+, fr7           ! 21 v+0 LU
    fmul    fr8, fr5            ! 21 v+0 MG2
    fmov.s  fr6, @-r6           ! 22 v+0 WB
    ftrv    xmtrx, fv0          ! 22 v+0 TR
    fmov.s  @r2, fr8            ! 23 v+0 LV
    add     #-4, r6             ! 23 v+0 D4
    movca.l r0, @r6             ! 24 v+0 M0
    add     r10, r3             ! 24 v+0 AC
    fmov.s  fr5, @r6            ! 25 v+0 WG
    fmov.s  fr4, @-r6           ! 26 v+0 WR
    fmov.s  fr8, @-r6           ! 27 v+0 WV
    add     r8, r1              ! 27 v+0 AP
    fmov.s  fr7, @-r6           ! 28 v+0 WU
    fmov.s  fr3, @-r6           ! 29 v+0 WW
    fmov.s  fr2, @-r6           ! 30 v+0 WZ
    fipr    fv0, fv0            ! 30 v+0 VF
    fmov.s  fr1, @-r6           ! 31 v+0 WY
    bf/s    .p1_kernel
    add     r9, r2              ! 31 v+0 AU
.p1_epilogue:
    fcmp/gt fr3, fr11           !  2 v-1 FC
    fmov.s  fr0, @-r6           !  4 v-1 WX
    movt    r0                  !  4 v-1 MT
    mov.l   r0, @(52, r6)       !  5 v-1 OK
    add     r0, r4              !  5 v-1 AV
    add     #116, r6            !  6 v-1 AO
    bra     .p1_done
    nop
.p1_single:
    tst     r7, r7
    bt      .p1_done
.p1_single_loop:
    fmov.s  @r5+, fr7           !  0 LDR
    fmov.s  @r5+, fr8           !  1 LDG
    fmov.s  @r5+, fr9           !  2 LDB
    fldi1   fr3                 !  3 F1
    add     #4, r5              !  3 AL4
    fmov.s  @r5+, fr4           !  6 LSR
    fmov.s  @r5+, fr5           !  7 LSG
    fmov.s  @r5, fr6            !  8 LSB
    movca.l r0, @r6             !  9 M1
    add     r12, r5             !  9 ALS
    fmov.s  fr6, @-r6           ! 10 WSB
    fmov.s  fr5, @-r6           ! 11 WSG
    fmov.s  fr4, @-r6           ! 12 WSR
    fmov.s  @r3+, fr4           ! 13 LCR
    fmov.s  @r3+, fr5           ! 14 LCG
    fmov.s  @r3+, fr6           ! 15 LCB
    fmul    fr12, fr4           ! 15 MR1
    fmov.s  @r3, fr10           ! 16 LCA
    fmul    fr13, fr5           ! 16 MG1
    fmov.s  @r1+, fr0           ! 17 L0
    fmul    fr14, fr6           ! 17 MB1
    fmov.s  @r1+, fr1           ! 18 L1
    fmul    fr15, fr10          ! 18 MA
    fmov.s  @r1, fr2            ! 19 L2
    fmul    fr7, fr4            ! 19 MR2
    fmov.s  fr10, @-r6          ! 20 WA
    fmul    fr9, fr6            ! 20 MB2
    fmov.s  @r2+, fr7           ! 21 LU
    fmul    fr8, fr5            ! 21 MG2
    fmov.s  fr6, @-r6           ! 22 WB
    ftrv    xmtrx, fv0          ! 22 TR
    fmov.s  @r2, fr8            ! 23 LV
    add     #-4, r6             ! 23 D4
    movca.l r0, @r6             ! 24 M0
    add     r10, r3             ! 24 AC
    fmov.s  fr5, @r6            ! 25 WG
    fmov.s  fr4, @-r6           ! 26 WR
    fmov.s  fr8, @-r6           ! 27 WV
    add     r8, r1              ! 27 AP
    fmov.s  fr7, @-r6           ! 28 WU
    fmov.s  fr3, @-r6           ! 29 WW
    fmov.s  fr2, @-r6           ! 30 WZ
    fipr    fv0, fv0            ! 30 VF
    fmov.s  fr1, @-r6           ! 31 WY
    add     r9, r2              ! 31 AU
    fcmp/gt fr3, fr11           ! 34 FC
    fmov.s  fr0, @-r6           ! 36 WX
    movt    r0                  ! 36 MT
    mov.l   r0, @(52, r6)       ! 37 OK
    add     r0, r4              ! 37 AV
    add     #116, r6            ! 38 AO
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

!
! void pvr_pack_sh4(const ClipVertex* in, pvr_vertex_packed_t* out, uint32_t n,
!                   const float* k)
!
! For each vertex writes the submit-ready PVR vertex - flags, screen x, y,
! 1/w, u, v, argb, oargb - to consecutive 32-byte slots of out (the
! clipping path works its clip position out again from the source).
! k = {half width, half height, 127.5, -127.5}. flags is
! PVR_CMD_VERTEX if the vertex is finite and in front of the near plane,
! else 0. Colours are assumed non-negative and clamped above at 1: with
! d = 127.5 (c - 1), trunc(|d| - d) = 255 (1 - min(c, 1)), and the packed
! words are built from those complements and inverted once at the end
! (which also makes the offset colour's alpha 0xFF).
!
! 52 cycles per vertex; one register set (fr1-11 data, fr0 = 127.5,
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
    mov     #1, r0
    cmp/hs  r0, r7
    bf      .pk_single
    fmov.s  @r1+, fr1           !  0 v+0 LX
    fmov.s  @r1+, fr2           !  1 v+0 LY
    fmov.s  @r1+, fr3           !  2 v+0 LZ
    fmov.s  @r1+, fr4           !  3 v+0 LW
    mov.l   @r1+, r10           !  4 v+0 LU
    mov.l   @r1+, r11           !  5 v+0 LV
    fadd    fr4, fr1            !  5 v+0 XW
    fmov.s  @r1+, fr6           !  6 v+0 LR
    fmul    fr0, fr6            !  8 v+0 MR
    fadd    fr4, fr3            !  9 v+0 VZ
    fmov.s  @r1+, fr8           ! 10 v+0 LG
    fmov    fr4, fr5            ! 11 v+0 QC
    fadd    fr12, fr6           ! 11 v+0 DR
    fmov.s  @r1+, fr10          ! 12 v+0 LB
    fmul    fr0, fr8            ! 12 v+0 MG
    fneg    fr2                 ! 13 v+0 YN
    fcmp/gt fr15, fr3           ! 13 v+0 FC
    fmov    fr6, fr7            ! 14 v+0 CR
    fmul    fr0, fr10           ! 14 v+0 MB
    fabs    fr7                 ! 15 v+0 FR
    fadd    fr12, fr8           ! 15 v+0 DG
    movca.l r0, @r2             ! 16 v+0 MC
    fsub    fr6, fr7            ! 16 v+0 SR
    mov.l   r11, @-r2           ! 17 v+0 WV
    fadd    fr4, fr2            ! 17 v+0 YW
    fmov    fr8, fr9            ! 18 v+0 CG
    fadd    fr12, fr10          ! 18 v+0 DB
    fabs    fr9                 ! 19 v+0 FG
    ftrc    fr7, fpul           ! 19 v+0 TR
    fmov.s  @r1+, fr6           ! 20 v+0 LA
    fsub    fr8, fr9            ! 20 v+0 SG
    fmov    fr10, fr11          ! 21 v+0 CB
    fmul    fr5, fr5            ! 21 v+0 QM
    fabs    fr11                ! 22 v+0 FB
    fmul    fr0, fr6            ! 22 v+0 MA
    sts     fpul, r6            ! 23 v+0 STR
    fsub    fr10, fr11          ! 23 v+0 SB
    ftrc    fr9, fpul           ! 24 v+0 TG
    movt    r5                  ! 24 v+0 MT
    sts     fpul, r9            ! 25 v+0 STG
    fadd    fr12, fr6           ! 25 v+0 DA
    ftrc    fr11, fpul          ! 26 v+0 TB
    shll8   r6                  ! 26 v+0 SHG
    fmov.s  @r1+, fr8           ! 27 v+0 LSR
    fsrra   fr5                 ! 27 v+0 QR
    fmov    fr6, fr7            ! 28 v+0 CA
    or      r9, r6              ! 28 v+0 ORG
    sts     fpul, r9            ! 29 v+0 STB
    fabs    fr7                 ! 30 v+0 FA
    shll8   r6                  ! 30 v+0 SHB
    fmov.s  @r1+, fr10          ! 31 v+0 LSG
    fsub    fr6, fr7            ! 31 v+0 SA
    fmul    fr0, fr8            ! 32 v+0 MSR
    or      r9, r6              ! 32 v+0 ORB
    mov.l   r10, @-r2           ! 33 v+0 WU
    fmul    fr0, fr10           ! 33 v+0 MSG
    fmov.s  fr5, @-r2           ! 34 v+0 WZ
    ftrc    fr7, fpul           ! 34 v+0 TA
    fmov.s  @r1+, fr6           ! 35 v+0 LSB
    fadd    fr12, fr8           ! 35 v+0 DSR
    mov.b   @r1, r4             ! 36 v+0 LOK
    fadd    fr12, fr10          ! 36 v+0 DSG
    sts     fpul, r9            ! 37 v+0 STA
    fmul    fr0, fr6            ! 37 v+0 MSB
    fmov    fr8, fr9            ! 38 v+0 CSR
    fmul    fr5, fr1            ! 38 v+0 KX
    fmov    fr10, fr11          ! 39 v+0 CSG
    fmul    fr5, fr2            ! 39 v+0 KY
    fabs    fr9                 ! 40 v+0 FSR
    fadd    fr12, fr6           ! 40 v+0 DSB
    fabs    fr11                ! 41 v+0 FSG
    fsub    fr8, fr9            ! 41 v+0 SSR
    fsub    fr10, fr11          ! 42 v+0 SSG
    shll16  r9                  ! 42 v+0 SHA1
    fmov    fr6, fr7            ! 43 v+0 CSB
    shll8   r9                  ! 43 v+0 SHA2
    fabs    fr7                 ! 44 v+0 FSB
    ftrc    fr9, fpul           ! 44 v+0 TSR
    sts     fpul, r8            ! 45 v+0 STSR
    fsub    fr6, fr7            ! 45 v+0 SSB
    ftrc    fr11, fpul          ! 46 v+0 TSG
    or      r9, r6              ! 46 v+0 ORA
    sts     fpul, r9            ! 47 v+0 STSG
    fmul    fr14, fr2           ! 47 v+0 HY
    fmul    fr13, fr1           ! 48 v+0 HX
    shll8   r8                  ! 48 v+0 SHSG
    fmov.s  fr2, @-r2           ! 49 v+0 WY
    ftrc    fr7, fpul           ! 49 v+0 TSB
    fmov.s  fr1, @-r2           ! 50 v+0 WX
    or      r9, r8              ! 50 v+0 ORSG
    sts     fpul, r9            ! 51 v+0 STSB
    add     #12, r1             ! 51 v+0 AI
    add     #-1, r7
    tst     r7, r7
    bt      .pk_epilogue
    .align 5
.pk_kernel:
    fmov.s  @r1+, fr1           !  0 v+0 LX
    and     r4, r5              !  0 v-1 VA
    fmov.s  @r1+, fr2           !  1 v+0 LY
    neg     r5, r5              !  1 v-1 VN
    fmov.s  @r1+, fr3           !  2 v+0 LZ
    and     r3, r5              !  2 v-1 VF
    fmov.s  @r1+, fr4           !  3 v+0 LW
    shll8   r8                  !  3 v-1 SHSB
    mov.l   @r1+, r10           !  4 v+0 LU
    not     r6, r6              !  4 v-1 NA
    mov.l   @r1+, r11           !  5 v+0 LV
    fadd    fr4, fr1            !  5 v+0 XW
    fmov.s  @r1+, fr6           !  6 v+0 LR
    or      r9, r8              !  6 v-1 ORSB
    mov.l   r5, @-r2            !  7 v-1 WF
    not     r8, r8              !  7 v-1 NO
    mov.l   r6, @(24, r2)       !  8 v-1 WA
    fmul    fr0, fr6            !  8 v+0 MR
    mov.l   r8, @(28, r2)       !  9 v-1 WO
    fadd    fr4, fr3            !  9 v+0 VZ
    fmov.s  @r1+, fr8           ! 10 v+0 LG
    add     #56, r2             ! 10 v-1 AO
    fmov    fr4, fr5            ! 11 v+0 QC
    fadd    fr12, fr6           ! 11 v+0 DR
    fmov.s  @r1+, fr10          ! 12 v+0 LB
    fmul    fr0, fr8            ! 12 v+0 MG
    fneg    fr2                 ! 13 v+0 YN
    fcmp/gt fr15, fr3           ! 13 v+0 FC
    fmov    fr6, fr7            ! 14 v+0 CR
    fmul    fr0, fr10           ! 14 v+0 MB
    fabs    fr7                 ! 15 v+0 FR
    fadd    fr12, fr8           ! 15 v+0 DG
    movca.l r0, @r2             ! 16 v+0 MC
    fsub    fr6, fr7            ! 16 v+0 SR
    mov.l   r11, @-r2           ! 17 v+0 WV
    fadd    fr4, fr2            ! 17 v+0 YW
    fmov    fr8, fr9            ! 18 v+0 CG
    fadd    fr12, fr10          ! 18 v+0 DB
    fabs    fr9                 ! 19 v+0 FG
    ftrc    fr7, fpul           ! 19 v+0 TR
    fmov.s  @r1+, fr6           ! 20 v+0 LA
    fsub    fr8, fr9            ! 20 v+0 SG
    fmov    fr10, fr11          ! 21 v+0 CB
    fmul    fr5, fr5            ! 21 v+0 QM
    fabs    fr11                ! 22 v+0 FB
    fmul    fr0, fr6            ! 22 v+0 MA
    sts     fpul, r6            ! 23 v+0 STR
    fsub    fr10, fr11          ! 23 v+0 SB
    ftrc    fr9, fpul           ! 24 v+0 TG
    movt    r5                  ! 24 v+0 MT
    sts     fpul, r9            ! 25 v+0 STG
    fadd    fr12, fr6           ! 25 v+0 DA
    ftrc    fr11, fpul          ! 26 v+0 TB
    shll8   r6                  ! 26 v+0 SHG
    fmov.s  @r1+, fr8           ! 27 v+0 LSR
    fsrra   fr5                 ! 27 v+0 QR
    fmov    fr6, fr7            ! 28 v+0 CA
    or      r9, r6              ! 28 v+0 ORG
    sts     fpul, r9            ! 29 v+0 STB
    dt      r7                  ! 29 loop count
    fabs    fr7                 ! 30 v+0 FA
    shll8   r6                  ! 30 v+0 SHB
    fmov.s  @r1+, fr10          ! 31 v+0 LSG
    fsub    fr6, fr7            ! 31 v+0 SA
    fmul    fr0, fr8            ! 32 v+0 MSR
    or      r9, r6              ! 32 v+0 ORB
    mov.l   r10, @-r2           ! 33 v+0 WU
    fmul    fr0, fr10           ! 33 v+0 MSG
    fmov.s  fr5, @-r2           ! 34 v+0 WZ
    ftrc    fr7, fpul           ! 34 v+0 TA
    fmov.s  @r1+, fr6           ! 35 v+0 LSB
    fadd    fr12, fr8           ! 35 v+0 DSR
    mov.b   @r1, r4             ! 36 v+0 LOK
    fadd    fr12, fr10          ! 36 v+0 DSG
    sts     fpul, r9            ! 37 v+0 STA
    fmul    fr0, fr6            ! 37 v+0 MSB
    fmov    fr8, fr9            ! 38 v+0 CSR
    fmul    fr5, fr1            ! 38 v+0 KX
    fmov    fr10, fr11          ! 39 v+0 CSG
    fmul    fr5, fr2            ! 39 v+0 KY
    fabs    fr9                 ! 40 v+0 FSR
    fadd    fr12, fr6           ! 40 v+0 DSB
    fabs    fr11                ! 41 v+0 FSG
    fsub    fr8, fr9            ! 41 v+0 SSR
    fsub    fr10, fr11          ! 42 v+0 SSG
    shll16  r9                  ! 42 v+0 SHA1
    fmov    fr6, fr7            ! 43 v+0 CSB
    shll8   r9                  ! 43 v+0 SHA2
    fabs    fr7                 ! 44 v+0 FSB
    ftrc    fr9, fpul           ! 44 v+0 TSR
    sts     fpul, r8            ! 45 v+0 STSR
    fsub    fr6, fr7            ! 45 v+0 SSB
    ftrc    fr11, fpul          ! 46 v+0 TSG
    or      r9, r6              ! 46 v+0 ORA
    sts     fpul, r9            ! 47 v+0 STSG
    fmul    fr14, fr2           ! 47 v+0 HY
    fmul    fr13, fr1           ! 48 v+0 HX
    shll8   r8                  ! 48 v+0 SHSG
    fmov.s  fr2, @-r2           ! 49 v+0 WY
    ftrc    fr7, fpul           ! 49 v+0 TSB
    fmov.s  fr1, @-r2           ! 50 v+0 WX
    or      r9, r8              ! 50 v+0 ORSG
    sts     fpul, r9            ! 51 v+0 STSB
    bf/s    .pk_kernel
    add     #12, r1             ! 51 v+0 AI
.pk_epilogue:
    and     r4, r5              !  0 v-1 VA
    neg     r5, r5              !  1 v-1 VN
    and     r3, r5              !  2 v-1 VF
    shll8   r8                  !  3 v-1 SHSB
    not     r6, r6              !  4 v-1 NA
    or      r9, r8              !  6 v-1 ORSB
    mov.l   r5, @-r2            !  7 v-1 WF
    not     r8, r8              !  7 v-1 NO
    mov.l   r6, @(24, r2)       !  8 v-1 WA
    mov.l   r8, @(28, r2)       !  9 v-1 WO
    add     #56, r2             ! 10 v-1 AO
    bra     .pk_done
    nop
.pk_single:
    tst     r7, r7
    bt      .pk_done
.pk_single_loop:
    fmov.s  @r1+, fr1           !  0 LX
    fmov.s  @r1+, fr2           !  1 LY
    fmov.s  @r1+, fr3           !  2 LZ
    fmov.s  @r1+, fr4           !  3 LW
    mov.l   @r1+, r10           !  4 LU
    mov.l   @r1+, r11           !  5 LV
    fadd    fr4, fr1            !  5 XW
    fmov.s  @r1+, fr6           !  6 LR
    fmul    fr0, fr6            !  8 MR
    fadd    fr4, fr3            !  9 VZ
    fmov.s  @r1+, fr8           ! 10 LG
    fmov    fr4, fr5            ! 11 QC
    fadd    fr12, fr6           ! 11 DR
    fmov.s  @r1+, fr10          ! 12 LB
    fmul    fr0, fr8            ! 12 MG
    fneg    fr2                 ! 13 YN
    fcmp/gt fr15, fr3           ! 13 FC
    fmov    fr6, fr7            ! 14 CR
    fmul    fr0, fr10           ! 14 MB
    fabs    fr7                 ! 15 FR
    fadd    fr12, fr8           ! 15 DG
    movca.l r0, @r2             ! 16 MC
    fsub    fr6, fr7            ! 16 SR
    mov.l   r11, @-r2           ! 17 WV
    fadd    fr4, fr2            ! 17 YW
    fmov    fr8, fr9            ! 18 CG
    fadd    fr12, fr10          ! 18 DB
    fabs    fr9                 ! 19 FG
    ftrc    fr7, fpul           ! 19 TR
    fmov.s  @r1+, fr6           ! 20 LA
    fsub    fr8, fr9            ! 20 SG
    fmov    fr10, fr11          ! 21 CB
    fmul    fr5, fr5            ! 21 QM
    fabs    fr11                ! 22 FB
    fmul    fr0, fr6            ! 22 MA
    sts     fpul, r6            ! 23 STR
    fsub    fr10, fr11          ! 23 SB
    movt    r5                  ! 24 MT
    ftrc    fr9, fpul           ! 24 TG
    sts     fpul, r9            ! 25 STG
    fadd    fr12, fr6           ! 25 DA
    shll8   r6                  ! 26 SHG
    ftrc    fr11, fpul          ! 26 TB
    fmov.s  @r1+, fr8           ! 27 LSR
    fsrra   fr5                 ! 27 QR
    fmov    fr6, fr7            ! 28 CA
    or      r9, r6              ! 28 ORG
    sts     fpul, r9            ! 29 STB
    fabs    fr7                 ! 30 FA
    shll8   r6                  ! 30 SHB
    fmov.s  @r1+, fr10          ! 31 LSG
    fsub    fr6, fr7            ! 31 SA
    or      r9, r6              ! 32 ORB
    fmul    fr0, fr8            ! 32 MSR
    mov.l   r10, @-r2           ! 33 WU
    fmul    fr0, fr10           ! 33 MSG
    fmov.s  fr5, @-r2           ! 34 WZ
    ftrc    fr7, fpul           ! 34 TA
    fmov.s  @r1+, fr6           ! 35 LSB
    fadd    fr12, fr8           ! 35 DSR
    mov.b   @r1, r4             ! 36 LOK
    fadd    fr12, fr10          ! 36 DSG
    sts     fpul, r9            ! 37 STA
    fmul    fr0, fr6            ! 37 MSB
    fmov    fr8, fr9            ! 38 CSR
    fmul    fr5, fr1            ! 38 KX
    fmov    fr10, fr11          ! 39 CSG
    fmul    fr5, fr2            ! 39 KY
    fabs    fr9                 ! 40 FSR
    fadd    fr12, fr6           ! 40 DSB
    fabs    fr11                ! 41 FSG
    fsub    fr8, fr9            ! 41 SSR
    shll16  r9                  ! 42 SHA1
    fsub    fr10, fr11          ! 42 SSG
    fmov    fr6, fr7            ! 43 CSB
    shll8   r9                  ! 43 SHA2
    fabs    fr7                 ! 44 FSB
    ftrc    fr9, fpul           ! 44 TSR
    sts     fpul, r8            ! 45 STSR
    fsub    fr6, fr7            ! 45 SSB
    or      r9, r6              ! 46 ORA
    ftrc    fr11, fpul          ! 46 TSG
    sts     fpul, r9            ! 47 STSG
    fmul    fr14, fr2           ! 47 HY
    fmul    fr13, fr1           ! 48 HX
    shll8   r8                  ! 48 SHSG
    fmov.s  fr2, @-r2           ! 49 WY
    ftrc    fr7, fpul           ! 49 TSB
    fmov.s  fr1, @-r2           ! 50 WX
    or      r9, r8              ! 50 ORSG
    sts     fpul, r9            ! 51 STSB
    add     #12, r1             ! 51 AI
    and     r4, r5              ! 52 VA
    neg     r5, r5              ! 53 VN
    and     r3, r5              ! 54 VF
    shll8   r8                  ! 55 SHSB
    not     r6, r6              ! 56 NA
    or      r9, r8              ! 58 ORSB
    mov.l   r5, @-r2            ! 59 WF
    not     r8, r8              ! 59 NO
    mov.l   r6, @(24, r2)       ! 60 WA
    mov.l   r8, @(28, r2)       ! 61 WO
    add     #56, r2             ! 62 AO
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
! FSRRAs: dirA (19 cycles/vertex, one set) does the dot products and
! parks x, z' and w in slots 14, 13, 12; dirB (17 cycles/vertex, two
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
    mov     #1, r0
    cmp/hs  r0, r7
    bf      .dira_single
    fmov.s  @r1+, fr4           !  0 v+0 LA0
    fmov.s  @r1+, fr5           !  1 v+0 LA1
    fmov.s  @r1+, fr6           !  2 v+0 LA2
    fmov.s  @r1+, fr7           !  3 v+0 LA3
    fmov.s  @r1+, fr0           !  4 v+0 LB0
    fmov.s  @r1+, fr1           !  5 v+0 LB1
    fmov.s  @r1+, fr2           !  6 v+0 LB2
    fipr    fv8, fv4            !  6 v+0 F1
    fmov.s  @r1, fr3            !  7 v+0 LB3
    add     #36, r1             !  8 v+0 AL
    fipr    fv8, fv0            ! 10 v+0 F2
    fmov    fr12, fr1           ! 11 v+0 CX
    fmov    fr7, fr2            ! 12 v+0 WC
    fadd    fr7, fr4            ! 12 v+0 Z1
    fabs    fr2                 ! 13 v+0 WA
    fmac    fr0, fr3, fr1       ! 14 v+0 X
    fmov.s  fr1, @-r2           ! 16 v+0 SX
    fsub    fr7, fr2            ! 16 v+0 WS
    fmov.s  fr4, @-r2           ! 17 v+0 SZ
    fmov.s  fr2, @-r2           ! 18 v+0 SW
    add     #-1, r7
    tst     r7, r7
    bt      .dira_epilogue
    .align 5
.dira_kernel:
    fmov.s  @r1+, fr4           !  0 v+0 LA0
    add     #76, r2             !  0 v-1 AS
    fmov.s  @r1+, fr5           !  1 v+0 LA1
    dt      r7                  !  1 loop count
    fmov.s  @r1+, fr6           !  2 v+0 LA2
    fmov.s  @r1+, fr7           !  3 v+0 LA3
    fmov.s  @r1+, fr0           !  4 v+0 LB0
    fmov.s  @r1+, fr1           !  5 v+0 LB1
    fmov.s  @r1+, fr2           !  6 v+0 LB2
    fipr    fv8, fv4            !  6 v+0 F1
    fmov.s  @r1, fr3            !  7 v+0 LB3
    add     #36, r1             !  8 v+0 AL
    fipr    fv8, fv0            ! 10 v+0 F2
    fmov    fr12, fr1           ! 11 v+0 CX
    fmov    fr7, fr2            ! 12 v+0 WC
    fadd    fr7, fr4            ! 12 v+0 Z1
    fabs    fr2                 ! 13 v+0 WA
    fmac    fr0, fr3, fr1       ! 14 v+0 X
    fmov.s  fr1, @-r2           ! 16 v+0 SX
    fsub    fr7, fr2            ! 16 v+0 WS
    fmov.s  fr4, @-r2           ! 17 v+0 SZ
    bf/s    .dira_kernel
    fmov.s  fr2, @-r2           ! 18 v+0 SW
.dira_epilogue:
    add     #76, r2             !  0 v-1 AS
    bra     .dira_done
    nop
.dira_single:
    tst     r7, r7
    bt      .dira_done
.dira_single_loop:
    fmov.s  @r1+, fr4           !  0 LA0
    fmov.s  @r1+, fr5           !  1 LA1
    fmov.s  @r1+, fr6           !  2 LA2
    fmov.s  @r1+, fr7           !  3 LA3
    fmov.s  @r1+, fr0           !  4 LB0
    fmov.s  @r1+, fr1           !  5 LB1
    fmov.s  @r1+, fr2           !  6 LB2
    fipr    fv8, fv4            !  6 F1
    fmov.s  @r1, fr3            !  7 LB3
    add     #36, r1             !  8 AL
    fipr    fv8, fv0            ! 10 F2
    fmov    fr12, fr1           ! 11 CX
    fmov    fr7, fr2            ! 12 WC
    fadd    fr7, fr4            ! 12 Z1
    fabs    fr2                 ! 13 WA
    fmac    fr0, fr3, fr1       ! 14 X
    fmov.s  fr1, @-r2           ! 16 SX
    fsub    fr7, fr2            ! 16 WS
    fmov.s  fr4, @-r2           ! 17 SZ
    fmov.s  fr2, @-r2           ! 18 SW
    add     #76, r2             ! 19 AS
    dt      r7
    bf      .dira_single_loop
.dira_done:
    ! --- dirB
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
    mov     #1, r0
    cmp/hs  r0, r7
    bt      .dirb_pipelined
    mov     r10, r3             ! fewer pairs than the pipeline is deep:
    bra     .dirb_single        ! all single
    nop
.dirb_pipelined:
    fmov.s  @r1+, fr0           !  0 v+0 LW
    fmov.s  @r1+, fr1           !  1 v+0 LZ
    fmov.s  @r1, fr2            !  2 v+0 LX
    fsrra   fr2                 !  5 v+0 RH
    fmul    fr2, fr1            ! 12 v+0 Z2
    fmov.s  fr0, @r2            ! 14 v+0 WW
    fmov    fr1, fr3            ! 15 v+0 HC
    fmul    fr14, fr0           ! 15 v+0 SW
    fabs    fr3                 ! 16 v+0 HA
    fmov.s  @r5+, fr4           ! 17 v+1 LW
    fsub    fr1, fr3            ! 17 v+0 HS
    fmov.s  @r5+, fr5           ! 18 v+1 LZ
    fmov.s  @r5, fr6            ! 19 v+1 LX
    fmov    fr3, fr8            ! 21 v+0 DC
    fsrra   fr6                 ! 22 v+1 RH
    add     #120, r1            ! 27 v+0 AL
    fmul    fr12, fr8           ! 27 v+0 DM
    fmul    fr6, fr5            ! 29 v+1 Z2
    fadd    fr13, fr8           ! 30 v+0 DA
    fmov.s  fr4, @r6            ! 31 v+1 WW
    fmul    fr0, fr3            ! 31 v+0 S1
    fmov    fr5, fr7            ! 32 v+1 HC
    fmul    fr14, fr4           ! 32 v+1 SW
    fabs    fr7                 ! 33 v+1 HA
    fmul    fr8, fr8            ! 33 v+0 DQ
    add     #-1, r7         ! kernel iterations = pairs - 1
    tst     r7, r7
    bt      .dirb_epilogue
    .align 5
.dirb_kernel:
    fmov.s  @r1+, fr0           !  0 v+0 LW
    fsub    fr5, fr7            !  0 v-1 HS
    fmov.s  @r1+, fr1           !  1 v+0 LZ
    dt      r7                  !  1 loop count
    fmov.s  @r1, fr2            !  2 v+0 LX
    fsrra   fr8                 !  2 v-2 DR
    fmov    fr7, fr9            !  4 v-1 DC
    fsrra   fr2                 !  5 v+0 RH
    add     #4, r2              !  9 v-2 A4
    fmul    fr8, fr3            !  9 v-2 S2
    add     #120, r5            ! 10 v-1 AL
    fmul    fr12, fr9           ! 10 v-1 DM
    fmov.s  fr3, @r2            ! 11 v-2 WS
    add     #124, r2            ! 12 v-2 AS
    fmul    fr2, fr1            ! 12 v+0 Z2
    fadd    fr13, fr9           ! 13 v-1 DA
    fmov.s  fr0, @r2            ! 14 v+0 WW
    fmul    fr4, fr7            ! 14 v-1 S1
    fmov    fr1, fr3            ! 15 v+0 HC
    fmul    fr14, fr0           ! 15 v+0 SW
    fabs    fr3                 ! 16 v+0 HA
    fmul    fr9, fr9            ! 16 v-1 DQ
    fmov.s  @r5+, fr4           ! 17 v+1 LW
    fsub    fr1, fr3            ! 17 v+0 HS
    fmov.s  @r5+, fr5           ! 18 v+1 LZ
    fmov.s  @r5, fr6            ! 19 v+1 LX
    fsrra   fr9                 ! 19 v-1 DR
    fmov    fr3, fr8            ! 21 v+0 DC
    fsrra   fr6                 ! 22 v+1 RH
    add     #4, r6              ! 26 v-1 A4
    fmul    fr9, fr7            ! 26 v-1 S2
    add     #120, r1            ! 27 v+0 AL
    fmul    fr12, fr8           ! 27 v+0 DM
    fmov.s  fr7, @r6            ! 28 v-1 WS
    add     #124, r6            ! 29 v-1 AS
    fmul    fr6, fr5            ! 29 v+1 Z2
    fadd    fr13, fr8           ! 30 v+0 DA
    fmov.s  fr4, @r6            ! 31 v+1 WW
    fmul    fr0, fr3            ! 31 v+0 S1
    fmov    fr5, fr7            ! 32 v+1 HC
    fmul    fr14, fr4           ! 32 v+1 SW
    fabs    fr7                 ! 33 v+1 HA
    bf/s    .dirb_kernel
    fmul    fr8, fr8            ! 33 v+0 DQ
.dirb_epilogue:
    fsub    fr5, fr7            !  0 v-1 HS
    fsrra   fr8                 !  2 v-2 DR
    fmov    fr7, fr9            !  4 v-1 DC
    add     #4, r2              !  9 v-2 A4
    fmul    fr8, fr3            !  9 v-2 S2
    add     #120, r5            ! 10 v-1 AL
    fmul    fr12, fr9           ! 10 v-1 DM
    fmov.s  fr3, @r2            ! 11 v-2 WS
    add     #124, r2            ! 12 v-2 AS
    fadd    fr13, fr9           ! 13 v-1 DA
    fmul    fr4, fr7            ! 14 v-1 S1
    fmul    fr9, fr9            ! 16 v-1 DQ
    fsrra   fr9                 ! 19 v-1 DR
    add     #4, r6              ! 26 v-1 A4
    fmul    fr9, fr7            ! 26 v-1 S2
    fmov.s  fr7, @r6            ! 28 v-1 WS
    add     #124, r6            ! 29 v-1 AS
.dirb_single:
    tst     r3, r3
    bt      .dirb_done
.dirb_single_loop:
    fmov.s  @r1+, fr0           !  0 LW
    fmov.s  @r1+, fr1           !  1 LZ
    fmov.s  @r1, fr2            !  2 LX
    fsrra   fr2                 !  5 RH
    fmul    fr2, fr1            ! 12 Z2
    fmov.s  fr0, @r2            ! 14 WW
    fmov    fr1, fr3            ! 15 HC
    fmul    fr14, fr0           ! 15 SW
    fabs    fr3                 ! 16 HA
    fsub    fr1, fr3            ! 17 HS
    fmov    fr3, fr8            ! 21 DC
    add     #56, r1             ! 27 AL
    fmul    fr12, fr8           ! 27 DM
    fadd    fr13, fr8           ! 30 DA
    fmul    fr0, fr3            ! 31 S1
    fmul    fr8, fr8            ! 33 DQ
    fsrra   fr8                 ! 36 DR
    add     #4, r2              ! 43 A4
    fmul    fr8, fr3            ! 43 S2
    fmov.s  fr3, @r2            ! 45 WS
    add     #60, r2             ! 46 AS
    dt      r3
    bf      .dirb_single_loop
.dirb_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    rts
    mov.l   @r15+, r8

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
! pointA (38 cycles/vertex, one register set) leaves x, z' and w in
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
    mov     #1, r0
    cmp/hs  r0, r7
    bf      .pta_single
    fldi1   fr8                 !  0 v+0 F8
    fmov.s  @r1+, fr0           !  3 v+0 LV
    fmov.s  @r1+, fr9           !  4 v+0 LP0
    fmov.s  @r1+, fr10          !  5 v+0 LP1
    fmov.s  @r1, fr11           !  9 v+0 LP2
    fmov    fr9, fr1            ! 11 v+0 C1
    fmov    fr11, fr3           ! 12 v+0 C3
    fmov    fr10, fr2           ! 14 v+0 C2
    fmov.s  @r3+, fr4           ! 15 v+0 LA0
    ftrv    xmtrx, fv8          ! 15 v+0 TR
    fmov.s  @r3+, fr5           ! 16 v+0 LA1
    fmov.s  @r3+, fr6           ! 17 v+0 LA2
    fmov.s  @r3, fr7            ! 18 v+0 LA3
    fipr    fv8, fv4            ! 22 v+0 PN
    fipr    fv8, fv0            ! 23 v+0 PP
    fipr    fv8, fv8            ! 24 v+0 LL
    fmov    fr12, fr1           ! 25 v+0 CX
    fmov    fr14, fr5           ! 27 v+0 AH
    fmov    fr11, fr15          ! 28 v+0 CL
    fsrra   fr11                ! 29 v+0 RL
    add     #52, r1             ! 34 v+0 AL
    add     #52, r3             ! 35 v+0 A3
    fmul    fr11, fr15          ! 35 v+0 Q
    fmul    fr11, fr7           ! 36 v+0 NL
    fmul    fr11, fr3           ! 37 v+0 PL
    add     #-1, r7
    tst     r7, r7
    bt      .pta_epilogue
    .align 5
.pta_kernel:
    fldi1   fr8                 !  0 v+0 F8
    fmul    fr13, fr15          !  0 v-1 T
    fmov    fr7, fr2            !  1 v-1 WC
    fadd    fr7, fr4            !  1 v-1 Z
    fabs    fr2                 !  2 v-1 WA
    fmac    fr0, fr3, fr1       !  2 v-1 X
    fmov.s  @r1+, fr0           !  3 v+0 LV
    fsub    fr15, fr5           !  3 v-1 AT
    fmov.s  @r1+, fr9           !  4 v+0 LP0
    dt      r7                  !  4 loop count
    fmov.s  @r1+, fr10          !  5 v+0 LP1
    fmov    fr5, fr6            !  6 v-1 AC
    fabs    fr6                 !  7 v-1 AB
    fsub    fr7, fr2            !  7 v-1 WS
    fmov.s  fr1, @-r2           !  8 v-1 SX
    fadd    fr5, fr6            !  8 v-1 AM
    fmov.s  @r1, fr11           !  9 v+0 LP2
    fmov.s  fr4, @-r2           ! 10 v-1 SZ
    fmov    fr9, fr1            ! 11 v+0 C1
    fmul    fr6, fr2            ! 11 v-1 WF
    fmov    fr11, fr3           ! 12 v+0 C3
    fmov.s  fr2, @-r2           ! 13 v-1 SW
    fmov    fr10, fr2           ! 14 v+0 C2
    add     #76, r2             ! 14 v-1 AS
    fmov.s  @r3+, fr4           ! 15 v+0 LA0
    ftrv    xmtrx, fv8          ! 15 v+0 TR
    fmov.s  @r3+, fr5           ! 16 v+0 LA1
    fmov.s  @r3+, fr6           ! 17 v+0 LA2
    fmov.s  @r3, fr7            ! 18 v+0 LA3
    fipr    fv8, fv4            ! 22 v+0 PN
    fipr    fv8, fv0            ! 23 v+0 PP
    fipr    fv8, fv8            ! 24 v+0 LL
    fmov    fr12, fr1           ! 25 v+0 CX
    fmov    fr14, fr5           ! 27 v+0 AH
    fmov    fr11, fr15          ! 28 v+0 CL
    fsrra   fr11                ! 29 v+0 RL
    add     #52, r1             ! 34 v+0 AL
    add     #52, r3             ! 35 v+0 A3
    fmul    fr11, fr15          ! 35 v+0 Q
    fmul    fr11, fr7           ! 36 v+0 NL
    bf/s    .pta_kernel
    fmul    fr11, fr3           ! 37 v+0 PL
.pta_epilogue:
    fmul    fr13, fr15          !  0 v-1 T
    fmov    fr7, fr2            !  1 v-1 WC
    fadd    fr7, fr4            !  1 v-1 Z
    fabs    fr2                 !  2 v-1 WA
    fmac    fr0, fr3, fr1       !  2 v-1 X
    fsub    fr15, fr5           !  3 v-1 AT
    fmov    fr5, fr6            !  6 v-1 AC
    fabs    fr6                 !  7 v-1 AB
    fsub    fr7, fr2            !  7 v-1 WS
    fmov.s  fr1, @-r2           !  8 v-1 SX
    fadd    fr5, fr6            !  8 v-1 AM
    fmov.s  fr4, @-r2           ! 10 v-1 SZ
    fmul    fr6, fr2            ! 11 v-1 WF
    fmov.s  fr2, @-r2           ! 13 v-1 SW
    add     #76, r2             ! 14 v-1 AS
    bra     .pta_done
    nop
.pta_single:
    tst     r7, r7
    bt      .pta_done
.pta_single_loop:
    fldi1   fr8                 !  0 F8
    fmov.s  @r1+, fr0           !  3 LV
    fmov.s  @r1+, fr9           !  4 LP0
    fmov.s  @r1+, fr10          !  5 LP1
    fmov.s  @r1, fr11           !  9 LP2
    fmov    fr9, fr1            ! 11 C1
    fmov    fr11, fr3           ! 12 C3
    fmov    fr10, fr2           ! 14 C2
    fmov.s  @r3+, fr4           ! 15 LA0
    ftrv    xmtrx, fv8          ! 15 TR
    fmov.s  @r3+, fr5           ! 16 LA1
    fmov.s  @r3+, fr6           ! 17 LA2
    fmov.s  @r3, fr7            ! 18 LA3
    fipr    fv8, fv4            ! 22 PN
    fipr    fv8, fv0            ! 23 PP
    fipr    fv8, fv8            ! 24 LL
    fmov    fr12, fr1           ! 25 CX
    fmov    fr14, fr5           ! 27 AH
    fmov    fr11, fr15          ! 28 CL
    fsrra   fr11                ! 29 RL
    add     #52, r1             ! 34 AL
    add     #52, r3             ! 35 A3
    fmul    fr11, fr15          ! 35 Q
    fmul    fr11, fr7           ! 36 NL
    fmul    fr11, fr3           ! 37 PL
    fmul    fr13, fr15          ! 38 T
    fmov    fr7, fr2            ! 39 WC
    fadd    fr7, fr4            ! 39 Z
    fabs    fr2                 ! 40 WA
    fmac    fr0, fr3, fr1       ! 40 X
    fsub    fr15, fr5           ! 41 AT
    fmov    fr5, fr6            ! 44 AC
    fabs    fr6                 ! 45 AB
    fsub    fr7, fr2            ! 45 WS
    fmov.s  fr1, @-r2           ! 46 SX
    fadd    fr5, fr6            ! 46 AM
    fmov.s  fr4, @-r2           ! 48 SZ
    fmul    fr6, fr2            ! 49 WF
    fmov.s  fr2, @-r2           ! 51 SW
    add     #76, r2             ! 52 AS
    dt      r7
    bf      .pta_single_loop
.pta_done:
    ! --- dirB
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
    mov     #1, r0
    cmp/hs  r0, r7
    bt      .ptb_pipelined
    mov     r10, r3             ! fewer pairs than the pipeline is deep:
    bra     .ptb_single         ! all single
    nop
.ptb_pipelined:
    fmov.s  @r1+, fr0           !  0 v+0 LW
    fmov.s  @r1+, fr1           !  1 v+0 LZ
    fmov.s  @r1, fr2            !  2 v+0 LX
    fsrra   fr2                 !  5 v+0 RH
    fmul    fr2, fr1            ! 12 v+0 Z2
    fmov.s  fr0, @r2            ! 14 v+0 WW
    fmov    fr1, fr3            ! 15 v+0 HC
    fmul    fr14, fr0           ! 15 v+0 SW
    fabs    fr3                 ! 16 v+0 HA
    fmov.s  @r5+, fr4           ! 17 v+1 LW
    fsub    fr1, fr3            ! 17 v+0 HS
    fmov.s  @r5+, fr5           ! 18 v+1 LZ
    fmov.s  @r5, fr6            ! 19 v+1 LX
    fmov    fr3, fr8            ! 21 v+0 DC
    fsrra   fr6                 ! 22 v+1 RH
    add     #120, r1            ! 27 v+0 AL
    fmul    fr12, fr8           ! 27 v+0 DM
    fmul    fr6, fr5            ! 29 v+1 Z2
    fadd    fr13, fr8           ! 30 v+0 DA
    fmov.s  fr4, @r6            ! 31 v+1 WW
    fmul    fr0, fr3            ! 31 v+0 S1
    fmov    fr5, fr7            ! 32 v+1 HC
    fmul    fr14, fr4           ! 32 v+1 SW
    fabs    fr7                 ! 33 v+1 HA
    fmul    fr8, fr8            ! 33 v+0 DQ
    add     #-1, r7         ! kernel iterations = pairs - 1
    tst     r7, r7
    bt      .ptb_epilogue
    .align 5
.ptb_kernel:
    fmov.s  @r1+, fr0           !  0 v+0 LW
    fsub    fr5, fr7            !  0 v-1 HS
    fmov.s  @r1+, fr1           !  1 v+0 LZ
    dt      r7                  !  1 loop count
    fmov.s  @r1, fr2            !  2 v+0 LX
    fsrra   fr8                 !  2 v-2 DR
    fmov    fr7, fr9            !  4 v-1 DC
    fsrra   fr2                 !  5 v+0 RH
    add     #4, r2              !  9 v-2 A4
    fmul    fr8, fr3            !  9 v-2 S2
    add     #120, r5            ! 10 v-1 AL
    fmul    fr12, fr9           ! 10 v-1 DM
    fmov.s  fr3, @r2            ! 11 v-2 WS
    add     #124, r2            ! 12 v-2 AS
    fmul    fr2, fr1            ! 12 v+0 Z2
    fadd    fr13, fr9           ! 13 v-1 DA
    fmov.s  fr0, @r2            ! 14 v+0 WW
    fmul    fr4, fr7            ! 14 v-1 S1
    fmov    fr1, fr3            ! 15 v+0 HC
    fmul    fr14, fr0           ! 15 v+0 SW
    fabs    fr3                 ! 16 v+0 HA
    fmul    fr9, fr9            ! 16 v-1 DQ
    fmov.s  @r5+, fr4           ! 17 v+1 LW
    fsub    fr1, fr3            ! 17 v+0 HS
    fmov.s  @r5+, fr5           ! 18 v+1 LZ
    fmov.s  @r5, fr6            ! 19 v+1 LX
    fsrra   fr9                 ! 19 v-1 DR
    fmov    fr3, fr8            ! 21 v+0 DC
    fsrra   fr6                 ! 22 v+1 RH
    add     #4, r6              ! 26 v-1 A4
    fmul    fr9, fr7            ! 26 v-1 S2
    add     #120, r1            ! 27 v+0 AL
    fmul    fr12, fr8           ! 27 v+0 DM
    fmov.s  fr7, @r6            ! 28 v-1 WS
    add     #124, r6            ! 29 v-1 AS
    fmul    fr6, fr5            ! 29 v+1 Z2
    fadd    fr13, fr8           ! 30 v+0 DA
    fmov.s  fr4, @r6            ! 31 v+1 WW
    fmul    fr0, fr3            ! 31 v+0 S1
    fmov    fr5, fr7            ! 32 v+1 HC
    fmul    fr14, fr4           ! 32 v+1 SW
    fabs    fr7                 ! 33 v+1 HA
    bf/s    .ptb_kernel
    fmul    fr8, fr8            ! 33 v+0 DQ
.ptb_epilogue:
    fsub    fr5, fr7            !  0 v-1 HS
    fsrra   fr8                 !  2 v-2 DR
    fmov    fr7, fr9            !  4 v-1 DC
    add     #4, r2              !  9 v-2 A4
    fmul    fr8, fr3            !  9 v-2 S2
    add     #120, r5            ! 10 v-1 AL
    fmul    fr12, fr9           ! 10 v-1 DM
    fmov.s  fr3, @r2            ! 11 v-2 WS
    add     #124, r2            ! 12 v-2 AS
    fadd    fr13, fr9           ! 13 v-1 DA
    fmul    fr4, fr7            ! 14 v-1 S1
    fmul    fr9, fr9            ! 16 v-1 DQ
    fsrra   fr9                 ! 19 v-1 DR
    add     #4, r6              ! 26 v-1 A4
    fmul    fr9, fr7            ! 26 v-1 S2
    fmov.s  fr7, @r6            ! 28 v-1 WS
    add     #124, r6            ! 29 v-1 AS
.ptb_single:
    tst     r3, r3
    bt      .ptb_done
.ptb_single_loop:
    fmov.s  @r1+, fr0           !  0 LW
    fmov.s  @r1+, fr1           !  1 LZ
    fmov.s  @r1, fr2            !  2 LX
    fsrra   fr2                 !  5 RH
    fmul    fr2, fr1            ! 12 Z2
    fmov.s  fr0, @r2            ! 14 WW
    fmov    fr1, fr3            ! 15 HC
    fmul    fr14, fr0           ! 15 SW
    fabs    fr3                 ! 16 HA
    fsub    fr1, fr3            ! 17 HS
    fmov    fr3, fr8            ! 21 DC
    add     #56, r1             ! 27 AL
    fmul    fr12, fr8           ! 27 DM
    fadd    fr13, fr8           ! 30 DA
    fmul    fr0, fr3            ! 31 S1
    fmul    fr8, fr8            ! 33 DQ
    fsrra   fr8                 ! 36 DR
    add     #4, r2              ! 43 A4
    fmul    fr8, fr3            ! 43 S2
    fmov.s  fr3, @r2            ! 45 WS
    add     #60, r2             ! 46 AS
    dt      r3
    bf      .ptb_single_loop
.ptb_done:
    fmov.s  @r15+, fr15
    fmov.s  @r15+, fr14
    fmov.s  @r15+, fr13
    fmov.s  @r15+, fr12
    mov.l   @r15+, r11
    mov.l   @r15+, r10
    mov.l   @r15+, r9
    rts
    mov.l   @r15+, r8
