# PCC Optimizer & PDP-11 Code Density

Status: **persisted plan, not implemented.** Targets the pcc tree the project
uses to compile `c99/` (per the Koitix decision: modern pcc, not tcc).

## 1. Two goals

1. **Close the code-size gap** between pcc's PDP-11 backend and the DMR
   compiler (dmr-cc) by improving the backend and folding in DMR's codegen
   choices. Measured today: pcc emits ~64% larger objects than dmr-cc, ~38%
   larger with `-O` (see [[pcc-vs-dmrcc-codegen]]).
2. **Make dead code elimination strong enough to replace `#ifdef`** in the
   `c99/` dialect work: fold compile-time constants and eliminate dead branches
   so `if (V7)`-style guards cost zero bytes.

## 2. Why these two are the same problem

The project's dialect is "select, not detect": `#ifdef` macros pick a code path,
and the unchosen path is never parsed, so it cannot be type-checked and its
provenance is fragile. If pcc folds constants and strips dead branches, the same
selection becomes `if (V7)` with `V7` a compile-time constant: the compiler sees
and type-checks every branch (the "detect" benefit), then DCE removes the dead
arm so the object is byte-identical to the `#ifdef` version. That only holds if
DCE is real, not cosmetic.

## 3. What the pasted analysis gets right, and what to correct

- **Right:** the size gap is Johnson's abstraction. Expression trees plus a
  table-driven code generator plus conservative frames plus naive register
  allocation, versus DMR's hand-tuned CISC templates.
- **Correct the metric:** the analysis benchmarks amd64/i386 against GCC `-Os` /
  LLVM `-Oz`. Our target is PDP-11 code density versus DMR, which is narrower
  and more tractable. We are not trying to beat GCC's amd64 backend; we are
  trying to make pcc's PDP-11 backend match DMR.
- **Already identified:** earlier measurement named the "unhonored" features:
  register struct pointers, bit/indexed addressing, and dead stores. These are
  Phase 1 below, not new discoveries.

## 4. Work, in order of leverage

### Phase 0: characterize the gap
- Build a harness that compiles the V7 corpus (`orig/` and `c99/`) with current
  pcc `-O` and with dmr-cc, diffs object sizes, and buckets the bloat
  (addressing modes, reloads, dead stores, prologue/epilogue, branch selection).
- Pin the exact pcc tree and the exact dmr-cc reference.
- Output: a per-cause breakdown that reorders everything below.

### Phase 1: backend codegen (highest leverage, already measured)
- Honor `register` on struct and pointer locals (pcc reloads them today).
- Emit PDP-11 indexed and bit-addressing modes where DMR does.
- Auto-increment / auto-decrement for pointer walks and loop idioms.
- Dead-store elimination (store to a location overwritten before any read).
- Branch selection: fold short-branch vs jump into codegen (partly done at the
  `as` level; move up).

### Phase 2: middle-end DCE (the `#ifdef` replacement)
- Constant propagation across basic blocks, not just within one expression tree.
- Dead-branch elimination: fold `if (const-expr)` and drop the unreachable arm.
- Acceptance: `if (V7)` with `V7` const emits no bytes for the false arm.

### Phase 3: peephole (fold DMR templates into `c2`)
- Extend `c2` (the peephole pass) with the DMR instruction sequences the backend
  cannot express structurally: inc/dec folded into memory operands, `sob` loops,
  branch-to-jump and jump-to-branch conversion, the DMR frame conventions.

### Phase 4: whole-program dead code (linker coordination)
- Per-function and per-data section placement in pcc (the function-sections
  analog), plus gc-sections in the PDP-11 `ld`. The biggest lift, and it needs
  linker changes; schedule last, after the per-translation-unit wins above.

## 5. "Incorporate DMR optimizations," concretely

A checklist, each matched against dmr-cc's actual emitted assembly for the
corpus. The `orig/` tree holds byte-identical goldens, so the diff is mechanical:

- Rich addressing modes: auto-inc/dec, indexed, register, immediate short forms.
- Register allocation that reserves the auto-inc/dec registers for pointer walks.
- Bit-field and shift generation.
- Loop idiom recognition (`sob` for count-down loops).
- The DMR stack/frame layout and short-branch selection.

## 6. Acceptance criteria
- pcc `-O` PDP-11 output within ~10% of dmr-cc on the corpus (down from ~64%).
- A compile-time-constant guard (`if (V7)`) produces no dead-branch bytes,
  verified by object size and a dump.
- A written migration note for `c99/`: replace `#ifdef` dialect guards with
  const-guarded `if`, keeping both arms type-checked.

## 7. Open questions
- Of the "unhonored" features, which are unimplemented in pcc's PDP-11 backend
  versus present-but-not-selected? (Phase 0 answers.)
- Where should DCE live: tree-level in the front/middle end before codegen, or
  assembly-level after? Placement decides how Phase 2 interacts with Phase 1.
- Does the target pcc already carry an interprocedural DCE pass we can enable,
  or must it be written?
- Link-time section GC depends on the V7 `ld` port; scope Phase 4 as its own
  project rather than folding it into this one.
