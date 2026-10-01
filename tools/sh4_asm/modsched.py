"""Modulo scheduling + code generation for the PVR lighting kernels
(simulant/renderers/pvr/pvr_lighting_sh4.s).

A kernel is described by a list of ops for ONE vertex:
    (name, group, text, [(dep, latency), ...])
where text is a format string over a register set (e.g. "{f0}", "{rp}") and
group (LS / FE / EX) only matters for phantom ops with no text. Consecutive
vertices alternate between `nsets` register sets, so a vertex may live up to
nsets*II cycles; REUSE lists (first writer, [readers]) pairs per register
that must not overlap that reuse.

Timing model: sh4timing.py, from the measured SH7091 tables at
https://ornio.nilware.io/sh4-sim/timings/. For a dependency that passes a
register (RAW), the latency comes from those tables for that producer and
consumer, not from the op list, whose number is then only used for ordering
edges (address-register updates, the FPUL chain, memory order). Issue is by
the measured classes, and the FP pipe's structural holds depend on what
follows (FTRV holds vector ops 4 cycles but scalar arithmetic 5, ...).

The loop's taken bf/s issues alone, 2 cycles after the instruction before
it; that is a fixed cost per kernel pass and isn't modelled here.
"""
import json
import os

import sh4timing as T

# Latencies for ordering edges that don't pass a register (or as a fallback);
# register dependencies take theirs from sh4timing.
LOAD, FLD, FTRV, FIPR, FSRRA, FMUL, FMUL_ST, MOV = 2, 1, 7, 4, 6, 3, 4, 1


def _insns(ops, sets):
    return {n: T.Insn(x.format(**sets)) for n, _, x, _ in ops if x}


def _classes(ops, sets):
    return {n: T.op_class(x.format(**sets) if x else "", g) for n, g, x, _ in ops}


def edge_latency(ins, d, n, given):
    """Issue distance an edge d -> n needs under the measured timings."""
    p, c = ins.get(d), ins.get(n)
    if p is None or c is None:
        return given
    raw = [r for r in p.writes if r in c.reads]
    if raw:
        return max(T.latency(p, r, c) for r in raw)
    waw = [r for r in p.writes if r in c.writes and r.startswith("fr")]
    if waw:
        return max(given, T.waw(p, c))
    return given


def solve(ops, fe_busy, reuse, II, quiet=False, time_limit=240, nsets=2, extra=None,
          sets=None):
    """fe_busy is unused (the FP pipe's holds come from sh4timing); it stays
    in the signature so the op tables keep documenting what they occupy."""
    from ortools.sat.python import cp_model
    sets = sets or {}
    ins = _insns(ops, sets)
    cls = _classes(ops, sets)
    assert "C3" not in cls.values(), "an op that issues alone can't be scheduled"
    m = cp_model.CpModel()
    H = nsets * II                              # register-set reuse distance
    t = {n: m.NewIntVar(0, 4 * II - 1, n) for n, *_ in ops}
    c = {}
    for n, g, _, deps in ops:
        for d, l in deps:
            m.Add(t[n] >= t[d] + edge_latency(ins, d, n, l))
        c[n] = m.NewIntVar(0, II - 1, "c" + n)
        m.AddModuloEquality(c[n], t[n], II)
    for w, rs in reuse:
        for r in rs:
            m.Add(t[r] < t[w] + H)
    at = {}
    for n, g, *_ in ops:
        for k in range(II):
            b = m.NewBoolVar(f"{n}@{k}")
            m.Add(c[n] == k).OnlyEnforceIf(b)
            m.Add(c[n] != k).OnlyEnforceIf(b.Not())
            at[n, k] = b
    fp = [n for n in cls if cls[n] == "C5" and n in ins]
    for k in range(II):
        m.Add(sum(at[n, k] for n, *_ in ops) <= 2)
        # C1, C2 and C5 never pair with their own class; C4 pairs with all
        for one in ("C1", "C2", "C5"):
            m.Add(sum(at[n, k] for n in cls if cls[n] == one) <= 1)
    # FP pipe holds: after A issues, B can't for hold(A, B) cycles
    for a in fp:
        for b in fp:
            h = T.hold(ins[a].mn, ins[b].mn)
            for j in range(1, min(h, II)):
                for k in range(II):
                    m.Add(at[a, k] + at[b, (k + j) % II] <= 1)
    if extra:
        extra(m, t, c, II)
    span = m.NewIntVar(0, 4 * II, "span")
    m.AddMaxEquality(span, list(t.values()))
    m.Minimize(span)
    s = cp_model.CpSolver()
    s.parameters.max_time_in_seconds = time_limit
    s.parameters.num_workers = 16
    r = s.Solve(m)
    if not quiet:
        print(f"  II={II} status", s.StatusName(r), flush=True)
    if r not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return None
    return {n: s.Value(t[n]) for n, *_ in ops}


def cached_solve(name, ops, fe_busy, reuse, II, nsets=2, extra=None, sets=None):
    """The solver is multithreaded and not deterministic, so the chosen
    schedule is cached next to this file; delete it to re-solve."""
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"{name}_ii{II}.json")
    if os.path.exists(path):
        sol = json.load(open(path))
    else:
        sol = solve(ops, fe_busy, reuse, II, quiet=True, nsets=nsets, extra=extra, sets=sets)
        assert sol, f"{name}: II={II} infeasible"
        json.dump(sol, open(path, "w"), indent=1)
    assert set(sol) == {n for n, *_ in ops}, "op list changed: delete " + path
    return sol


def gen_loop(ops, sets, sol, II, label, single_subst=(), dt_after=()):
    """Returns (prologue, kernel, epilogue, single, ip) as lists of asm lines.

    With len(sets) == k, the kernel is k*II cycles and starts vertices
    k*i .. k*i+k-1, vertex j using set j % k; it expects the iteration count
    in r7 and branches to .<label>_kernel. The prologue is the first `ip`
    iterations with only vertices >= 0, the epilogue the iterations after the
    last with only vertices < k*iters, so iters >= ip is required.
    `single` is one vertex, unpipelined, with `single_subst` (old, new) text
    replacements applied (pointer steps). The loop's dt goes after the ops
    named in `dt_after` (e.g. to keep it clear of other T-bit users)."""
    k = len(sets)
    K = k * II
    grp = {n: g for n, g, _, _ in ops}
    cls = _classes(ops, sets[0])
    txt = {n: x for n, _, x, _ in ops}

    def iteration(keep):
        out = []
        for c in range(K):
            cyc = []
            for n, t in sol.items():
                if (c - t) % II or not txt[n]:      # empty text: phantom op
                    continue
                dv = (c - t) // II
                if not keep(dv):
                    continue
                line = f"    {txt[n].format(**sets[dv % k]):<28}! {c:2d} v{dv:+d} {n}"
                cyc.append((grp[n] != "LS", line, n))
            out += [(c, l, n) for _, l, n in sorted(cyc)]
        return out

    def dvs():
        return [(c - t) // II for n, t in sol.items() for c in range(K) if (c - t) % II == 0]

    ip = next(i for i in range(16) if all(k * i + dv >= 0 for dv in dvs()))
    pro = []
    for i in range(ip):
        pro += iteration(lambda dv, i=i: k * i + dv >= 0)
    ker = iteration(lambda dv: True)
    epi = []
    for j in range(16):
        it = iteration(lambda dv, j=j: k * j + dv <= -1)
        if not it:
            break
        epi += it

    used = {}
    for c, _, n in ker:
        used.setdefault(c, []).append(cls[n])
    if "DTS" in sol:
        # a phantom op reserved the loop's dt slot
        dt_c = sol["DTS"] % II + II * (k - 1)
    else:
        after = max([sol[n] % II + II * (k - 1) for n in dt_after] or [-1])
        dt_c = next(c for c in range(K)
                    if c > after and len(used.get(c, [])) < 2 and "C2" not in used.get(c, []))
    kl = [l for _, l, _ in ker]
    pos = max([i for i, (c, _, _) in enumerate(ker) if c <= dt_c] or [-1]) + 1
    kl.insert(pos, f"    {'dt      r7':<28}! {dt_c:2d} loop count")
    last = kl.pop()
    assert "dt " not in last
    kl += [f"    bf/s    .{label}_kernel", last]

    single = []
    for n, t in sorted(sol.items(), key=lambda x: (x[1], grp[x[0]] != "LS")):
        if not txt[n]:
            continue
        x = txt[n].format(**sets[0])
        for a, b in single_subst:
            x = x.replace(a, b)
        single.append(f"    {x:<28}! {t:2d} {n}")
    return ([l for _, l, _ in pro], kl, [l for _, l, _ in epi], single, ip)
