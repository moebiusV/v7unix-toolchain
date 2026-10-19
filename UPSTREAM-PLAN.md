# Getting our pcc work upstream: what Anders Magnusson accepts, and a plan

Written from the whole upstream history (5,481 non-merge commits, 2002-2026, `PortableCC/pcc`), the GitHub
pull requests and issues of `PortableCC/pcc`, and a check of each of our candidates against **current upstream
master (9a993a5f)**.  Facts are marked as such; inferences say "I read this as".

## 1. How patches reach upstream (facts)

* **Anders (`ragge0`) is the only committer and applies almost everything himself.**  4,282 of the commits are his
  by author; the rest of the credit is in the message: "From Brian Callahan.", "Patches by Tim Kelly.",
  "Patch from Odidev."  329 messages carry such a credit (found by a rough pattern, so a lower bound); 413 mention a Jira ticket (`PCC-nnn`).
* **Commit messages are one line.**  98% have no body at all; the median subject is 48 characters; 73% end with a
  full stop; the verbs are Fix / Add / Remove / Do / Make / Allow / Handle / Use.  Known shortcomings go in the
  subject as `XXX - ...` ("Allow for keeping a zero in simple ops if targets want it. From Michal Pleban. XXX - will
  go away in the future.").
* **Commits are small.**  Median 10 lines changed, 75th percentile 45, 90th percentile 159.  66% touch one file and
  87% one area.  Commits over 3,000 lines are 34 of 5,481 and are mostly new files: a new port, or a new subsystem such as
  softfloat (RISC-V 5,690 lines from Tim Kelly, Intel 8086 5,143 from Alan Cox, MIPS64 4,814 from Brian
  Callahan, AArch64 5,157 from Odidev, Z8001 7,764 from Michal Pleban).
* **External contributions are also small.**  The 666 commits with a credit to someone else or a ticket reference have a
  median of 8 lines and a 90th percentile of 56.
* **GitHub pull requests that merged were tiny and merged within days.**  PR #1, #2, #4, #5, #8, #9, #14, #15,
  #17 (June 2025): 1 to 4 commits, 1 to 92 lines changed, 1 to 3 files (one is a regenerated `configure`, 1,041 lines), merged in 0 to 12
  days, five of them within a single week.  #49 (Pleban, 40 lines changed) merged in 3 days.
* **Big pull requests are not merged; Anders re-applies them by hand in pieces.**  #50 (Z8001, 78 commits): "Since it
  is a giant change I have to do it manually (by using diff and patch) in smaller pieces", then a list of one line
  per piece with a verdict: "(applied)", "(applied with changes)", "(rejected, calling routines already did that)".
  #47 (AArch64 overhaul, 27 commits, +1,930/-1,627) has been open since February 2026; Anders likes it ("Wow,
  impressing patch!") and has not had time.  #42 (IRIX, 20 lines) took ten months.
* **Reviewers outside ragge ask for squashing.**  On #50 Peter Metzger: "71 commits here seems a bit extreme?  Could
  you squash these down to a set that functionally describe what a reviewer actually needs to look at?"
* **Cosmetic mass changes are not wanted.**  #41 (convert 153 `//` comments in 42 files) was closed the day it
  opened, unmerged.
* **Anders edits what he applies and credits it anyway**: "applied with changes", "From Michal Pleban, with
  changes from me of how declarations are handled."
* **The compiler's tests are a separate repository**, `PortableCC/pcc-tests` (library work goes to `pcc-libs`);
  only the preprocessor has a test directory in the pcc tree.  I did not find a convention of adding a test with a
  compiler change, so put the repro in the PR text and offer it to `pcc-tests` separately.
* **Availability comes in bursts.**  He started a new job in January 2026; the June 2025 week and September to
  October 2026 (Z8001) are the active periods.  The mailing list (`pcc-list`) and GitHub issues are both used.
* **pdp11 and vax are his own ports**: 70 of 73 pdp11 commits and 97 of 110 vax commits are his; the last pdp11
  commit was August 2023.

I read this as: small, single-purpose, one-line-message patches against current master are what gets merged, quickly;
anything large needs to be cut into pieces that stand alone, and a new port can be added as new files in one go.

## 2. What our own history looks like

124 fine-grained commits on the original branch (kept locally as `fix-orig`, `e721812d`): pdp11 57 (median 51 lines),
c2 peephole 22 (median 112), front end 11 (median 24), vax 10 (median 103), shared back end `mip/` 7 (median 97),
x86 7 (median 201), AArch64 4 (median 1,862, one of 5,963).  The squashed `master` is good for reading and wrong for
upstreaming: upstream wants the small commits, so cut from `fix-orig`, not from `master`.

## 3. Candidates, checked against current upstream (9a993a5f)

"Still broken upstream" means our regression test fails on a stock build of 9a993a5f.

### Wave 1: small bug fixes found by fuzzing and by running real programs (propose first, one PR each)

| fix | our commit | size | still broken upstream? | proposed subject |
|---|---|---|---|---|
| `soft_cmp` orders negative and zero constants wrongly | `ab5fd377` | 8 lines | **yes** (`fpconstcmp` fails on amd64 and i386) | Fix soft_cmp for negative constants. |
| `x<<0` loses the node's type (compile error); merged constant left shifts go past the width (wrong code) | `4b5dc62d` | 31 lines | **yes** (`shiftzero`: compiler error; `shiftmerge`: wrong result) | Keep the type when removing a shift by zero. |
| amd64/i386: `int / int` and `int % int` retyped across an unsigned conversion; `(long)(int<<c)` | `3d7d83f4` | 10 lines | **yes** (`divretype`, `shiftwiden` on amd64: wrong result) | Do not retype DIV/MOD across the unsigned conversion. |
| GNU `case LOW ... HIGH:` expands one switch entry per value, so a huge range hangs the compiler; signed compare for an unsigned switch | (this session) | 85 lines | **yes** (`caserange`, `caserange64` hang) | Handle large GNU case ranges without expanding them. |
| pdp11: `sizeof` unsigned; DIV/MOD retype; long-to-long SCONV; long left shift names the wrong register pair; long register pairs start at an even register | `8b7bb43d` `b26427aa` `4dedb238` `2f981a3d` `6959fad5` | 2, 2, 4, 2, 11 lines | **to check** with the pdp11 tests against a stock pdp11 build | one commit each |
| `(a != b) == 0` and `(a == b) != 1` give the opposite answer (`andorbr()` flips the inner comparison in two of the four cases) | `fdb68ae9` | 18 lines | **yes** (`eqnefold`: wrong result on amd64 and aarch64 stock) | Fix removal of redundant EQ/NE nodes. |
| a decimal floating constant near the underflow limit is zero (`4.94e-324`) or rounded twice (`decbig()` tests the scaled exponent; `str2num()` rounds before `make()`) | `softfloat` fix, this session | 60 lines | **yes** (`a64subnormal`, with a binary64 `long double`: fails on stock aarch64, arm, mips) | Round subnormal decimal constants once. |
| `regs.c` `dce()` deletes a live loop assignment after removing a one-statement block | `ecb00296` | 31 lines | **not reproduced on x86** (`dceloop` passes on stock): check pdp11 and vax before proposing | |

Not to propose, already upstream: the one-line old-style-parameter struct descriptor (`s->sss = p->pss`, upstream
03ecc1d5, 2 October 2026), branch inversion that keeps operand order (upstream rewrote `deljumps` the same way),
tail merging on the intermediate code (upstream `comjump`/`xjump`, 18 October 2026).

### Wave 2: front-end and shared optimizations (each with a before/after code sample in the PR text)

All behind a flag so default output does not change: ours are under `-xdmr`; **ask Anders what he wants the flag
to be called, or whether they should join `-xtemps`/`-xdce`.**  Candidates, each 20 to 120 lines: reads of `const`
integer objects with a constant initializer fold to the constant (`-xdce`); `c ? a : b` in a condition branches
from each arm; a `?:` statement evaluates its arms for effect only; `a[i] op= v` computes the address once;
`x op= e` in a condition is a statement then a compare; a register already in place ranks cheaper in the matcher
(`match.c`, 39 lines); sharing spill slots between non-overlapping temporaries (`regs.c`, 97 lines); `retfold`.
Also a candidate: our cross-jumping variant (jump plus fall-through; 1% smaller on amd64 and AArch64 on top of
upstream's, never larger on 126 files), proposed as an extension of upstream's `comjump` rather than a rival.

### Wave 3: PDP-11 and VAX (his ports, his taste)

57 pdp11 and 10 vax commits, mostly 20 to 100 lines each (auto-increment addressing, byte operations, in-place
updates, jump tables, 64-bit `long long` through helper calls).  Offer them as themed series of small PRs rather
than the 3,000-line squashes; ask first whether he wants them.  The `cc2` peephole ports (`cc2/cc2.pdp11.c`, 
`cc2/cc2.vax.c`) fit infrastructure he added in 2021 for targets that need a cc2; they are new files and can be
proposed whole.

### Wave 4: AArch64 and x86

**Conflicts with open PR #47** (hakanrw, an overhaul of the existing back end, waiting for Anders since February).
Ours is a new back end with a text optimizer (5,963 lines in one file at its largest).  Do not open a competing PR
blind: compare the two (ours passes 66 emulator tests and compiles 846 of 900 corpus files), then either offer to
review #47 and port our `peep.h` and varargs on top of it, or ask Anders which he prefers.  x86: small
`-xdmr` items (`leal sym(,%reg,scale)`, shortest constants), 30 to 100 lines each.

## 4. Things to reconcile before proposing anything

* **K&R initializers.**  Our scanner rule (`int a[] { 1, 2 };`) now overlaps with Pleban's "accept ancient-style
  array init without braces" and "declarations without a type" (applied by Anders on 2 October).  Re-run the V7 sources
  on upstream alone; drop whichever of ours upstream now covers.
* **`relops(p, cookie)`** was changed upstream and we merged to it; our PDP-11 `if (--x >= 0)` case sits inside it.
* **Our `-O` for PDP-11 means "ccom's optimizer, then c2".**  The upstream tree has no `c2` for the PDP-11 (its `cc2/` holds only `cc2.nova.c`).
* **Known defect of ours** before any VAX/c2 PR: `cc2.vax.c` cross-jumps into the previous function's tail
  (`x509_crt.c` assembles on neither the host port nor real 32V).

## 5. How to run it

1. One branch per change, from current upstream master, named for the change, one commit, message in his style:
   one line, a full stop, no body; put the repro and the numbers in the PR description.
2. Cut each commit from `fix-orig` (`git cherry-pick`, then squash the pieces of one idea), re-verify on a stock
   build first (the table above), then run all the gates on the result.  Keep the diff to the lines that matter.
3. No more than two or three PRs open at once, small ones first: Wave 1 is five or six PRs of under 100 lines
   each.  Wait for each to merge or be commented on before sending the next group.
4. Mention the one-line summary on `pcc-list` as well; Anders reads both.
5. Say plainly what is not done ("XXX" in the subject is his own convention).
6. Our own earlier PR (#52, the relational operand swap) was withdrawn by us the same day because the real cause
   was elsewhere; the follow-up was never posted.  Close the loop in the first message ("that was a symptom; the
   cause is fixed upstream by your relops change / here is the actual fix").

## 6. Open questions for Anders

* What should our optimizations be called and gated by (`-xdmr`, or the existing `-x` flags)?
* Does he want the PDP-11 and VAX improvements at all, and in what grouping?
* AArch64: his preference between #47 and our back end.
* Whether a `cc2` for the VAX/PDP-11 belongs in the tree.
