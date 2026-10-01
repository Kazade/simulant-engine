"""In-order dual-issue timing model of the SH7091 from the measured tables at
https://ornio.nilware.io/sh4-sim/timings/ (mode M0, single precision, warm
caches). Used by modsched.py as its timing model, and to simulate a generated
loop body and report its steady-state cycles per pass
(python tools/sh4_asm/sh4timing.py <file.s>).

Rules applied:
  * issue classes: C1 (loads/stores, FP moves), C2 (int ALU), C5 (FP arith)
    never pair with their own class; C4 (mov, tst, cmp, not-taken branches)
    pairs with anything but C3; C3 (taken branches, system regs) issues alone.
  * result latencies by producer and consumer kind (Latency tables).
  * FPU structural holds (Structural blocking): after FTRV, vector ops
    (FIPR/FSRRA/FTRV) wait 4 and scalar FP arith 5; after FSRRA 3 / 4; after
    FIPR scalar FP arith waits 2.
  * a taken branch issues alone and no earlier than 2 cycles after the
    instruction before it (Consumer-side holds).
"""
import re
import sys

C1 = {"fmov.s", "fmov", "fldi0", "fldi1", "fabs", "fneg", "flds", "fsts", "lds",
      "sts", "mov.l", "mov.b", "mov.w", "movca.l", "pref", "ocbwb", "lds.l"}
C2 = {"add", "and", "or", "xor", "not", "neg", "sub", "shll", "shll2", "shll8",
      "shll16", "shlr", "shlr2", "shlr8", "shlr16", "shld", "shad", "movt", "dt",
      "extu.b", "extu.w", "exts.b", "exts.w", "swap.b", "swap.w", "xtrct"}
C4 = {"tst", "cmp/hs", "cmp/eq", "cmp/gt", "cmp/ge", "cmp/hi", "cmp/pz", "cmp/pl",
      "nop", "clrt", "sett", "bt", "bf"}   # mov Rm,Rn handled below
C5 = {"fadd", "fsub", "fmul", "fmac", "fipr", "ftrv", "fsrra", "fcmp/gt",
      "fcmp/eq", "ftrc", "float", "fdiv", "fsqrt", "fsca"}
VEC = {"fipr", "ftrv", "fsrra"}
SCAL_ARITH = {"fadd", "fsub", "fmul", "fmac", "fcmp/gt", "fcmp/eq", "ftrc", "float"}


def regs_of(op):
    op = op.strip()
    m = re.fullmatch(r"fv(\d+)", op)
    if m:
        b = int(m.group(1))
        return [f"fr{b+i}" for i in range(4)]
    m = re.fullmatch(r"dr(\d+)", op)
    if m:
        b = int(m.group(1))
        return [f"fr{b}", f"fr{b+1}"]
    if re.fullmatch(r"(fr|r)\d+", op) or op in ("fpul",):
        return [op]
    return []


class Insn:
    def __init__(self, text):
        self.text = text
        parts = text.split(None, 1)
        self.mn = parts[0]
        ops = [o.strip() for o in re.split(r",(?![^()]*\))", parts[1])] if len(parts) > 1 else []
        self.reads, self.writes = [], []
        mn = self.mn
        self.taken = False
        if mn == "mov" and ops and not ops[0].startswith("#"):
            self.cls = "C4"
        elif mn in C1:
            self.cls = "C1"
        elif mn in C2 or mn == "mov":
            self.cls = "C2"
        elif mn in C4:
            self.cls = "C4"
        elif mn in C5:
            self.cls = "C5"
        elif mn in ("bf/s", "bt/s", "bra"):
            self.cls = "C3"
            self.taken = True
        else:
            raise ValueError("unknown " + text)
        self.kind = mn
        # operands
        def mem(o):
            # returns (address regs read, address regs written)
            m = re.fullmatch(r"@(-?)(r\d+)(\+?)", o)
            if m:
                r = m.group(2)
                return [r], [r] if (m.group(1) or m.group(3)) else []
            m = re.fullmatch(r"@\((-?\d+),\s*(r\d+)\)", o)
            if m:
                return [m.group(2)], []
            m = re.fullmatch(r"@\(r0,\s*(r\d+)\)", o)
            if m:
                return ["r0", m.group(1)], []
            return None
        if mn in ("fmov.s", "fmov", "mov.l", "mov.b", "mov.w", "movca.l", "mov"):
            src, dst = ops
            ms, md = mem(src), mem(dst)
            if ms:
                self.reads += ms[0]; self.writes += ms[1] + regs_of(dst)
                self.addr_writes = ms[1]
                self.load = True
            elif md:
                self.reads += regs_of(src) + md[0]; self.writes += md[1]
                self.addr_writes = md[1]
                self.store = True
            else:
                if not src.startswith("#") and not src.startswith("."):
                    self.reads += regs_of(src)
                self.writes += regs_of(dst)
        elif mn == "pref":
            self.reads += mem(ops[0])[0]
        elif mn in ("fldi0", "fldi1"):
            self.writes += regs_of(ops[0])
        elif mn in ("fabs", "fneg", "fsrra"):
            self.reads += regs_of(ops[0]); self.writes += regs_of(ops[0])
        elif mn in ("fadd", "fsub", "fmul"):
            self.reads += regs_of(ops[0]) + regs_of(ops[1]); self.writes += regs_of(ops[1])
        elif mn == "fmac":
            self.reads += regs_of(ops[0]) + regs_of(ops[1]) + regs_of(ops[2]); self.writes += regs_of(ops[2])
        elif mn == "ftrv":
            self.reads += regs_of(ops[1]); self.writes += regs_of(ops[1])
        elif mn == "fipr":
            a, b = regs_of(ops[0]), regs_of(ops[1])
            self.reads += a + b; self.writes += [b[3]]
        elif mn in ("fcmp/gt", "fcmp/eq"):
            self.reads += regs_of(ops[0]) + regs_of(ops[1]); self.writes += ["T"]
        elif mn == "ftrc":
            self.reads += regs_of(ops[0]); self.writes += ["fpul"]
        elif mn in ("sts",):
            self.reads += ["fpul"]; self.writes += regs_of(ops[1])
        elif mn in ("fsts",):
            self.reads += ["fpul"]; self.writes += regs_of(ops[1])
        elif mn in ("lds",):
            self.reads += regs_of(ops[0]); self.writes += ["fpul"]
        elif mn in ("flds",):
            self.reads += regs_of(ops[0]); self.writes += ["fpul"]
        elif mn == "lds.l":
            self.reads += mem(ops[0])[0]; self.writes += mem(ops[0])[1] + ["fpul"]
            self.addr_writes = mem(ops[0])[1]
        elif mn in ("add", "and", "or", "xor", "sub"):
            if ops[0].startswith("#"):
                self.reads += regs_of(ops[1])
            else:
                self.reads += regs_of(ops[0]) + regs_of(ops[1])
            self.writes += regs_of(ops[1])
        elif mn in ("not", "neg", "extu.b", "extu.w", "exts.b", "exts.w", "swap.b", "swap.w"):
            self.reads += regs_of(ops[0]); self.writes += regs_of(ops[1])
        elif mn.startswith("shl") or mn.startswith("sha"):
            self.reads += sum((regs_of(o) for o in ops), []); self.writes += regs_of(ops[-1])
        elif mn == "movt":
            self.reads += ["T"]; self.writes += regs_of(ops[0])
        elif mn == "dt":
            self.reads += regs_of(ops[0]); self.writes += regs_of(ops[0]) + ["T"]
        elif mn in ("tst", "cmp/hs", "cmp/eq", "cmp/gt", "cmp/ge", "cmp/hi"):
            self.reads += sum((regs_of(o) for o in ops), []); self.writes += ["T"]
        elif mn in ("bt", "bf", "bf/s", "bt/s"):
            self.reads += ["T"]
        elif mn in ("bra", "nop"):
            pass
        else:
            raise ValueError("unhandled " + text)

    def ptype(self):
        mn = self.mn
        if getattr(self, "load", False):
            return "fload" if mn == "fmov.s" else "iload"
        if mn in ("fmov", "fldi0", "fldi1", "fabs", "fneg", "fsts"):
            return "fmove"
        if mn in ("fadd", "fsub", "fmul", "fmac", "float"):
            return "farith"
        return mn


def latency(p, preg, c):
    """Cycles from p's issue until c may issue, for c reading preg written by p."""
    if preg in getattr(p, "addr_writes", ()):
        return 1          # post-increment / pre-decrement address update
    pt = p.ptype()
    store = getattr(c, "store", False)
    cvec = c.mn in VEC
    if pt == "fload":
        return 3 if cvec else 2
    if pt == "fmove":
        return 3 if cvec else 1
    if pt == "farith":
        return 2 if store else 3
    if pt == "fipr":
        return 3 if store else 4
    if pt == "fsrra":
        return 5 if store else 6
    if pt == "ftrv":
        e = p.writes.index(preg)
        if store:
            return 5 if e < 3 else 6
        if cvec:
            return 7
        return (5, 5, 6, 7)[e]
    if pt in ("fcmp/gt", "fcmp/eq"):
        if c.taken:
            return 3
        return 2
    if pt == "ftrc":
        return 1 if c.mn == "sts" else 3
    if pt == "sts":
        return 3
    if pt == "iload":
        return 2
    if pt in ("lds", "flds"):
        return 3
    return 1


def waw(p, c):
    """Cycles from p's issue until c may issue, c overwriting an FP register p
    writes (and c doesn't read). Moves and loads may overwrite sooner than
    arithmetic (FSRRA / FTRV / FIPR form pages)."""
    pt = p.ptype()
    mv = c.ptype() in ("fload", "fmove")
    if pt == "fsrra":
        return 4            # an arithmetic writer is held 4 anyway (hold())
    if pt == "ftrv":
        return 5
    if pt == "fipr":
        return 2 if mv else 4
    return 1


def hold(a, b):
    """Structural: cycles after FP op `a` (mnemonic) issues before FP op `b`
    may issue, with no data dependence between them."""
    if a == "ftrv":
        return 4 if b in VEC else 5 if b in SCAL_ARITH else 1
    if a == "fsrra":
        return 3 if b in VEC else 4 if b in SCAL_ARITH else 1
    if a == "fipr":
        return 2 if b in SCAL_ARITH else 1
    return 1


def op_class(text, group=None):
    """Issue class of an instruction; a phantom op (no text) takes the class
    its scheduler group stands for."""
    if not text:
        return {"LS": "C1", "EX": "C2", "FE": "C5"}[group]
    return Insn(text).cls


def simulate(body, iters=40):
    """Steady-state cycles per pass of a loop body (list of instruction
    texts, ending with the taken branch and its delay slot)."""
    insns = [Insn(t) for t in body]
    ready = {}        # reg -> (producer insn, issue cycle)
    fp_last = []      # (mnemonic, cycle) of recent FP-pipe ops
    cyc = 0
    slot_used = []    # classes issued in the current cycle
    prev_issue = -10
    starts = []
    for it in range(iters):
        starts.append(None)
        for ins in insns:
            t = cyc if len(slot_used) < 2 else cyc + 1
            for r in ins.reads:
                if r in ready:
                    p, pc = ready[r]
                    t = max(t, pc + latency(p, r, ins))
            for r in ins.writes:
                if r in ready and r.startswith("fr") and r not in ins.reads:
                    p, pc = ready[r]
                    t = max(t, pc + waw(p, ins))
            if ins.cls == "C5":
                for a, ac in fp_last:
                    t = max(t, ac + hold(a, ins.mn))
            if ins.taken:
                t = max(t, prev_issue + 2)
            while True:
                if t > cyc:
                    cyc, slot_used = t, []
                ok = len(slot_used) < 2
                if ok and slot_used:
                    a, b = slot_used[0], ins.cls
                    if "C3" in (a, b) or (a == b and a != "C4"):
                        ok = False
                if ok:
                    break
                t = cyc + 1
            if starts[-1] is None:
                starts[-1] = cyc
            slot_used.append(ins.cls)
            if ins.taken:
                slot_used = ["C3", "C3"]   # issues alone
            for r in ins.writes:
                ready[r] = (ins, cyc)
            if ins.cls == "C5":
                fp_last = [(a, ac) for a, ac in fp_last if ac > cyc - 6] + [(ins.mn, cyc)]
            prev_issue = cyc
    d = [b - a for a, b in zip(starts[10:], starts[11:])]
    return sum(d) / len(d)


def kernel_bodies(path):
    lines = open(path).read().split("\n")
    out = {}
    for i, l in enumerate(lines):
        m = re.match(r"\.(\w+)_kernel:", l)
        if not m:
            continue
        body = []
        j = i + 1
        while True:
            s = lines[j].split("!")[0].strip()
            j += 1
            if not s:
                continue
            body.append(s)
            if s.startswith("bf/s"):
                body.append(lines[j].split("!")[0].strip())
                break
        out[m.group(1)] = body
    return out


if __name__ == "__main__":
    path = sys.argv[1]
    for name, body in kernel_bodies(path).items():
        n = len(body)
        print(f"{name:6s} insns={n:3d} cycles/iter={simulate(body):6.2f}")
