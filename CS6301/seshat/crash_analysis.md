# Seshat Fuzzing Crash Analysis

**Run summary:** 24 hours, ~9.1M execs at ~102 exec/sec, 8 crashes + 1 hang found, 1.29% edge coverage across 16,444 edges.

---

## Bug 1 — Integer divide-by-zero in centroid computation (SIGFPE)

**Crashes:** `id:000000`, `id:000001`, `id:000002`

**Location:** `sample.cc:96–97`

```c
// sample.cc:90–97
int np;
for(np=0; np < dataon[i]->getNpuntos() ; np++) {
    dataon[i]->cx += pto->x;
    dataon[i]->cy += pto->y;
}
dataon[i]->cx /= np;   // ← SIGFPE when np == 0
dataon[i]->cy /= np;
```

`Stroke::cx` and `cy` are `int` fields (`stroke.h:48`), so dividing by zero is an integer division — SIGFPE, not an IEEE 754 infinity. This fires whenever a stroke has `NP=0`, because the loop never runs and `np` is left at 0.

The three crashes reach this differently:

| Crash | Mutation | How `NP=0` is reached |
|---|---|---|
| `id:000000` | `nstrokes` field = `"X"` | `fscanf("%d")` fails; `nstrokes` retains a stale nonzero value from a prior loop iteration; subsequent `npuntos` read also fails and retains `0` from prior state → `Stroke(0, fd)` |
| `id:000001` | First `npuntos` field = `"R7"` | `nstrokes=8` parses fine; `npuntos` read fails on `"R"` and retains `0` → `Stroke(0, fd)` |
| `id:000002` | One `npuntos` field = `" 0"` | Explicit zero → `Stroke(0, fd)` — cleanest, fully deterministic reproduction |

Crash 002 is the canonical reproducer. Crashes 000/001 are state-dependent (rely on the AFL persistent-mode loop leaving stale values in the stack frame).

**Fix:** validate `npuntos > 0` in `loadSCGInk` before calling `new Stroke(...)`, or guard the division with `if (np > 0)` before lines 96–97.

---

## Bug 2 — Float-to-int overflow → heap buffer out-of-bounds in render pipeline (SIGABRT via ASAN)

**Crashes:** `id:000003`, `id:000004`, `id:000005`, `id:000006`, `id:000007`

This is a two-stage bug:

**Stage 1 — Float-to-int overflow in `render()` bounding box** (`sample.cc:657–660`):

```c
// xMAX, yMAX etc. are all int
if( pto->x > xMAX ) xMAX = pto->x;   // implicit float→int cast
if( pto->y > yMAX ) yMAX = pto->y;   // UB if float > INT_MAX
```

Coordinate values stored as `float` in `Punto` can exceed `INT_MAX`. Assigning e.g. `8.56e17f` to `int yMAX` is undefined behaviour (produces `INT_MIN` on x86 via saturating VCVTTSS2SI). The same overflow occurs in `Stroke::Stroke(int, FILE*)` for `rx/ry/rs/rt`.

**Stage 2 — Rendering formula produces wild pixel index:**

The corrupted `yMAX`/`yMIN` integer overflow then cascades through:
```c
int H = yMAX - yMIN + 1;       // another overflow
float R = (float)W / H;         // garbage ratio
...
aux.y = 5 + (H-10) * (float)(pto->y - yMIN) / (yMAX - yMIN + 1);
// denominator overflows again; aux.y becomes ~1e11
img[(int)aux.y][(int)aux.x] = 0;   // ← OOB write, ASAN catches → SIGABRT
```

The five crashes reach stage 1 via different sources of extreme floats:

| Crash | Source of extreme coordinate |
|---|---|
| `id:000003` | Explicit mutation: y-value `"855555555555555555"` (≈8.56×10¹⁷) written into a coordinate field. Direct, deterministic. |
| `id:000004–000007` | Descended from corpus entry `src:000198` (`"SCG_INK\n17 11i\n..."`). `fscanf("%d")` reads `nstrokes=17`, stops at space; the `"11i"` suffix causes subsequent `npuntos` reads to stall on `'i'`; all `Stroke(11, fd)` point reads also stall, leaving `Punto.x/y` as uninitialized heap bytes. After ~8 hours of persistent-mode execution, those heap slots contain garbage floats that overflow the `int` bounding box. |

There is also a **secondary, independent out-of-bounds bug** in `linea()` (`sample.cc:626–630`): the function draws 3×3 pixel neighborhoods around each point without any bounds check, so pixels at the image edge write one pixel outside the allocation. It was not the direct trigger here (the render formula keeps pixels in `[5, W-5]` under normal arithmetic), but it will fire for any input that legitimately places a point exactly at the boundary.

**Fix:** Change `xMAX`, `yMAX`, `xMIN`, `yMIN` in `render()` to `float`; add bounds clamping before `img[y][x]` writes; add `max(0, min(H-1, y))` / `max(0, min(W-1, x))` guards in `linea()`.

---

## Bug 3 — Resource exhaustion / hang (no validation on `nstrokes`)

**Hang:** `id:000000` (timeout ~360 s)

```
SCG_INK
170 34         ← nstrokes=170 parsed fine
400 33         ← npuntos[0]=400
...
```

`loadSCGInk()` (`sample.cc:126–131`) does no sanity check on `nstrokes` or `npuntos` before looping. With `nstrokes=170` and large `npuntos` values, `render()` draws a huge number of strokes and segments — `linea()` runs 320 iterations per segment (`dl = 3.125e-3`), so `170 × 400 × 320 = ~21M` pixel-write operations per exec. AFL's 40-second timeout fires. Note that `total_tmout: 1,490,372` in the fuzzer stats indicates this class of input hit the timeout millions of times throughout the run.

**Fix:** Add a hard cap on `nstrokes` and `npuntos` in `loadSCGInk` (e.g., reject inputs where either exceeds a reasonable bound like 10 000).

---

## Summary

| ID(s) | Signal | Unique bug | Root cause location |
|---|---|---|---|
| 000000, 000001, 000002 | SIGFPE (8) | Integer divide-by-zero | `sample.cc:96–97` — centroid `cx /= np` with `np=0` |
| 000003 | SIGABRT (6) | Float→int overflow → OOB | `sample.cc:657–660` — bounding box int assignment of large float |
| 000004–000007 | SIGABRT (6) | Same overflow, uninitialized source | Same location; floats come from uninitialized `Punto` heap bytes |
| hang 000000 | — | Resource exhaustion | `sample.cc:126–131` — uncapped `nstrokes`/`npuntos` |

**3 distinct root-cause bugs.** The SIGABRT cluster (crashes 3–7) is one bug class with two different ways to feed it extreme float values.
