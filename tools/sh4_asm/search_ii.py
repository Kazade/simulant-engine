"""Find the smallest feasible II for each kernel under modsched's timing
model, caching each schedule found (<name>_ii<II>.json).

    python tools/sh4_asm/search_ii.py [kernel ...]    (needs ortools)
"""
import json
import math
import os
import sys
import time

import lightgen as G
import modsched as M
import sh4timing as T

HERE = os.path.dirname(os.path.abspath(__file__))


def kernels():
    c1o, c1b, c1r = G.comb_ops(1)
    c2o, c2b, c2r = G.comb_ops(2)
    pko, pkb, pkr = G.pk_ops()
    return {
        "geo": (G.GEO_OPS, G.GEO_REUSE, 2, None, G.GEO_SETS[0], G.GEO_II),
        "comb2": (c2o, c2r, 2, None, G.COMB_SETS[0], G.COMB_II[2]),
        "comb1": (c1o, c1r, 2, None, G.COMB_SETS[0], G.COMB_II[1]),
        "p1": (G.P1_OPS, G.P1_REUSE, 1, G.p1_extra, {}, G.P1_II),
        "pk": (pko, pkr, 1, G.pk_extra, {}, G.PK_II),
        "dira": (G.DIRA_OPS, G.DIRA_REUSE, 1, None, {}, G.DIRA_II),
        "dirb": (G.DIRB_OPS, G.DIRB_REUSE, 2, None, G.DIRB_SETS[0], G.DIRB_II),
        "pta": (G.PTA_OPS, G.PTA_REUSE, 1, None, {}, G.PTA_II),
    }


def lower_bound(ops, sets):
    cls = M._classes(ops, sets)
    ins = M._insns(ops, sets)
    n = {k: sum(1 for v in cls.values() if v == k) for k in ("C1", "C2", "C5")}
    fp = [x for x in ins.values() if x.cls == "C5"]
    # each FP op keeps the next FP op off for at least its smallest hold
    hold = sum(min(T.hold(a.mn, b.mn) for b in fp) for a in fp)
    return max(n["C1"], n["C2"], n["C5"], math.ceil(len(ops) / 2), hold)


def main(names):
    ks = kernels()
    for name in names or ks:
        ops, reuse, nsets, extra, sets, old = ks[name]
        lb = lower_bound(ops, sets)
        print(f"{name}: old II {old}, resource bound {lb}", flush=True)
        for II in range(max(lb, old - 4), old + 12):
            path = os.path.join(HERE, f"{name}_ii{II}.json")
            t0 = time.time()
            sol = M.solve(ops, None, reuse, II, quiet=True, time_limit=120,
                          nsets=nsets, extra=extra, sets=sets)
            print(f"  II={II}: {'feasible' if sol else 'no'} ({time.time() - t0:.0f}s)",
                  flush=True)
            if sol:
                json.dump(sol, open(path, "w"), indent=1)
                break


if __name__ == "__main__":
    main(sys.argv[1:])
