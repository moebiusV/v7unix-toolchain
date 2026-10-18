# Old K&R forms in the V7 and 32V sources that pcc does not accept

pcc compiles the C of the 1989 standard and later, plus the K&R-1 conveniences that
survived into C89 (old-style function definitions with a separate parameter
declaration list, implicit `int`, calls to functions that were never declared).  The
sources of V7 and 32V were written for compilers that accepted more.  This file
lists every form we found that pcc rejects, what it means, where the original
sources use it, what supporting it would take, and what supporting it would break
for programs that are valid C99.

**The C dialect was deliberately not changed** (no `-xknr` mode exists).  Nothing
here is implemented; the last column of each section says what it would cost, so
the decision can be made with the numbers in front of it.

## How the numbers were made

* V7 (PDP-11), `tools/pccdiff.sh ../orig/usr/src/cmd`: 84 source files in `cmd/`.
  4 are fragments that other files `#include` (`lex/header.c`, `lex/ldefs.c`,
  `lex/once.c`, `cpp/yylex.c`), 52 compile, **28 are rejected**.  All 89 files of
  `libc` compile.  The driver passes `-Dunix`, as V7's `cc` did (without it the
  lex sources fail at `#ifdef unix` and look like a language problem; they are not).
* 32V (VAX), a loop over every `cmd/*.c`, `cmd/*/*.c` and `libc/*/*.c` (605 files)
  through the VAX ccom: **18 are rejected**, of which 3 are not language forms
  (see the end).
* Only the **first** error of a file is classified, so a rejected file can hide
  more of the same or another form behind it.  Fixing a form can therefore move a
  file from "rejected" to "rejected for the next form", not to "compiles".

| form | V7 cmd | 32V | detail |
|---|---|---|---|
| 1. members live in one namespace; `->` and `.` on anything | 20 | 5 | below |
| 2. an initializer written without `=` | 3 | 5 | below |
| 3. the old assignment operators `=+ =- =^ =<<` ... | 2 | 0 | below |
| 4. an undeclared name is an `extern int`; an `extern` in a block stays visible | 3 | 3 | below |
| 5. `unsigned` in front of a typedef name | 0 | 1 | below |
| 6. a pointer as an operand of `<<` | 0 | 1 | below |
| not a language form (missing header, fragment) | 0 | 3 | below |

(V7: 20 + 3 + 2 + 3 = 28.  32V: 5 + 5 + 3 + 1 + 1 + 3 = 18.)

---

## 1. Members belong to the program, not to the struct

**What it is.**  In the first C compilers a structure member name was an entry in
one table for the whole program: a name, an offset, a type.  `p->m` and `s.m` were
`*(char*)&p + offset-of-m`, whatever `p` pointed to and whatever `s` was.  So a
program could say `->` on a `char *`, `.` on a `double`, or use a member of one
struct through a pointer to another, and it meant "this offset, this type".  A
declaration of a struct or union with no tag and no variable
(`struct { short hiword; short loword; };`) did nothing but enter two member names
into the table.  C89 made members belong to their struct (and the `.`/`->`
operand must be a struct or a pointer to one).

**Where.**  V7 `cmd`: 20 files, three groups.

* Member of another struct, through a pointer of the wrong struct type:
  `c/c00.c:137` `rp->subsp = NULL;` (`rp` is a `struct hshtab *`; `subsp` is a member
  of `struct tname`, `c/c0.h:61`), and the same in `c01 c02 c03 c04 c10 c11 c12`;
  `icheck.c:355` `sblock.s_nfree = buf.df_nfree;`; `mkfs.c:110` `filsys.s_fsize = n;`.
* `->` on a pointer to something that is not a struct: `sh/args.c:108`
  `np->doluse=1;` (`np` is a `STRING *`, `doluse` is a member of `struct dolnod`),
  `sh/blok.c:83` `Lcheat((--p)->word) &= ~BUSY;`, `sh/word.c:43`
  `argp->argval[0]` (`argp` is a `char *`), `sh/cmd.c`, `sh/expand.c`, `sh/service.c`,
  `sh/xec.c`, `adb/print.c`, `adb/runpcs.c`.
* `.` on a value that is not a struct: `adb/format.c:107` `fw.sb=get(...)` where
  `fw` is a `double`.

32V: `icheck.c`, `restor.c` (`df_nfree`), `mkfs.c` (`s_fsize`), `ld.c:43`
`t=p->hiword; p->hiword=p->loword;` with the tagless
`struct {short hiword; short loword;};` of line 37 and `p` a `long *`,
`refer/hunt5.c:25` `master.b[i]` where `master` is a *pointer* to a union (`.` on a
pointer).

**What it takes.**  `buildtree()` in `cc/ccom/trees.c`, case `STREF`, rejects the
tree with `struct or union required` or `member '%s' not declared` (the lookup is
`findmember(strmemb(l->n_td->ss), name)`).  To accept the old forms:
(a) when the struct lookup fails, search a program-wide table of every member ever
declared, keyed by name (built where a struct body is completed, `pftn.c`
`dclstruct`), and take its offset and type if all entries with that name agree, or
if there is one; (b) for `.`, take the address of a non-struct lvalue first; for `->`
on a non-pointer, treat the value as a `char *`; (c) let a tagless, declarator-less
`struct{...};` enter its members instead of warning "declaration does not declare
anything".  About 150 lines, all in pass 1.

**What it breaks.**  Valid C99 programs are untouched if the global table is only a
fallback after the real lookup fails, because C99 rejects every one of these
trees.  What is lost is diagnostics: a typo that names a member of some other
struct, or a pointer of the wrong type, compiles silently and reads the wrong
offset.  In C99 two structs may reuse a member name at different offsets, which
makes the global lookup ambiguous (it must then be an error, so most of the V7
uses would still work only because their member names happen to be unique).  It
should warn under `-Wall`.

---

## 2. An initializer written without `=`

**What it is.**  Before `=` was required, a declarator could be followed directly
by its initial value: `int sect DEFSECTION;`, `char *man { "mrxtdp" };`.  The
`=` was added in the C of V7's successors because `int x -1;` and function
definitions (`int f() {`) read too alike.

**Where.**  V7: `adb/output.c:18` `CHAR *printptr printbuf;`, `adb/setup.c:36`
`POS *endhdr &corhdr[512];`, `lex/once.c`, lines 47-49 (included by `lex/lmain.c`):
`int ZCH NCH;`, `FILE *fout NULL, *errorf {stdout};`, `int sect DEFSECTION;`.  32V: `ar11.c:60` `char *man { "mrxtdp" };`, and in `refer`:
`deliv1.c:17` `struct _iobuf *fa 0;`, `kaiser.c:18` `int state 1000, i, book, ...;`,
`thash.c:59` `int k 0;`, `what2.c:24` `int ns 0;`.

**What it takes.**  `init_declarator` in `cc/ccom/cgram.y`: after a declarator that is
not a function declarator, a token that can begin an initializer (a constant, a
string, an identifier, `{`, `&`, `-`, `*`, `(`) starts it as if `=` had been
there.  A declarator that *is* a function declarator followed by `{` stays a
function body.  The grammar will report shift/reduce conflicts on `{` and on `(`
that need `%prec`; a typedef name as the next token also needs the lexer's
typedef-name feedback to stay correct.  Small, but it touches the declaration
grammar, which is the most fragile part of `cgram.y`.

**What it breaks.**  Nothing that is valid C99 changes meaning, since `int x 5;`
is a syntax error there.  The cost is error recovery and diagnostics: `int x y;`
used to be an error pointing at `y`; it would become an initializer that is
"undeclared `y`", and a missing `=` in ordinary code stops being reported as a
missing `=`.

---

## 3. The old assignment operators `=+ =- =* =/ =% =& =| =^ =<< =>>`

**What it is.**  Compound assignment used to be spelled with the operator after the
`=`.  `x =+ 1` was `x += 1`.  V7's own compiler already warned about the
ambiguity, which is why the order was reversed in C: `x =-1` is "assign minus one"
in C99 and "decrement `x`" in the old spelling.

**Where.**  V7: `adb/expr.c:214` `IF d==1 THEN expv =<<16; FI`, `lex/sub1.c:190`
`symbol[i] =^ 1;`.  None among the 32V first errors.

**What it takes.**  A lexer rule: `=` immediately followed by `+ - * / % & | ^ << >>`
returns the compound-assignment token.  It cannot be done as a rule, because the
text that C99 reads as two tokens (`x=-1`, `p=*q`, `a=&b`, `n=+5`) is the same
text.  The only safe form is "operator spelled `=op` and followed by a space", which
matches the two sources above (`=<<16` does not; it would need the digit rule) and
nothing else is guaranteed.

**What it breaks.**  If it is on for ordinary code, `x=-1`, `p=*q` and `a=&b` change
meaning silently.  That is the reason not to support it: **translate the two
sources** (`=<<` to `<<=`, `=^` to `^=`) instead.

---

## 4. Undeclared names are `extern int`; a block-level `extern` outlives its block

**What it is.**  Two separate rules of the old compilers.

(a) A name that was never declared and is not called was an `extern int` of
that name, to be resolved by the linker; only calls were checked for being
declared at all.  And an `int` could be indexed or dereferenced (`yyextra[m]`
with `yyextra` undeclared is "the int `yyextra` used as an address").

(b) A declaration made with `extern` (or a function declaration) inside a function
body entered the program-wide table and stayed in it, so a later function could
use the name without declaring it again.

**Where.**  V7: `lex/lib/reject.c:30` `if (yyextra[m])` (`yyextra` is defined by the
generated lexer; nothing in this file declares it); `make/main.c:260` has
`extern int yylineno;` inside one function and uses it at line 304 in another;
`make/dosys.c:51` declares `int intrupt();` in `await()` and uses it at line 110 in
another function.  32V: `init.c:40` `int reset();` inside `main()` and a use at
line 61 elsewhere; `refer/glue4.c:77` `gfile[0]`; `refer/mkey2.c:55` `n<minlen`
(names defined in other files).

**What it takes.**  (a) In `nametree()` (`trees.c`, where `%s undefined` is
reported), instead of an error, enter the name as `extern int` at file scope with a
warning.  (b) Do not remove an `extern` declaration from the symbol table when its
block closes (`symtabs.c`, block exit); in effect, file scope for every `extern`.

**What it breaks.**  (a) is the dangerous one: every typo in an identifier becomes a
link error instead of a compile error, which gives up the single most useful
diagnostic a C compiler has.  C99 removed implicit declarations of functions for
that reason and compilers now warn or error on them.  (b) breaks scoping: an
`extern int x;` in one function would be visible in the rest of the file, so a
later `x` that C99 treats as a different object or as undeclared (an error) would
silently refer to the first.  Neither affects valid C99, but (a) undoes
diagnostics that C99 users rely on; if wanted it should be a per-file option.

---

## 5. `unsigned` in front of a typedef name

**What it is.**  `unsigned` meant "the unsigned form of the following type", and
that included a typedef name: after `typedef short UNIT;`, `unsigned UNIT ccnt;`
is an `unsigned short`.

**Where.**  32V `cmd/prof/prof.c:10` and `:194`.  (None in V7.)

**What it takes.**  In the declaration-specifier code (`pftn.c`, `typwalk`/the
specifier merge), when `unsigned` or `signed` is followed by a typedef name,
replace the typedef by its underlying integer type and apply the signedness.

**What it breaks.**  In C99 `unsigned UNIT ccnt;` declares an `unsigned int`
variable named `UNIT` and then fails on `ccnt`; at block scope that is valid
(it shadows the typedef) so accepting the old reading changes the meaning of a
valid, if strange, program.  It would have to apply only when the next token after
the typedef name is a declarator, which is the exact case C99 rejects.

---

## 6. A pointer as an operand of a shift

**What it is.**  Pointers and integers were interchangeable in arithmetic:
`(mproc.p_addr+swplo)<<9` shifts the result of adding a (long) offset to a
pointer.

**Where.**  32V `cmd/ps.slow.c:403`.

**What it takes.**  `tymatch()` in `trees.c`: allow `<<` and `>>` with a pointer
left operand and convert it to `unsigned long` first.

**What it breaks.**  Nothing valid; it accepts what C99 calls a constraint
violation.  Low value.

---

## What is not a language form

* **Missing include directory.**  32V `cmd/nm.c:152` uses `STABTYPE` from
  `<stab.h>`, which the include directory used for these runs does not have.
* **Fragments.**  32V `cmd/cpp/yylex.c` (`OROR`, `ANDAND`) and `cmd/lex/once.c`
  (`NCH`) are `#include`d into other files and are not translation units.
* **Predefined names.**  V7's driver defined `unix` and `pdp11` (32V: `unix`, `vax`);
  pcc's preprocessor defines `pdp11` or `vax` but not `unix`.  Compile with
  `-Dunix`.
* **A struct tag called `void`.**  V7's `sh` has `struct void`; `void` is a keyword
  in pcc.  `tools/pccdiff.sh` compiles those with `-Dvoid=Void_`.

## What pcc accepts from the same era

Old-style function definitions (`f(a, b) int a; char *b; { ... }`), implicit `int`
for functions and variables, a call to an undeclared function (an implicit
`int f()`), `register` parameters, octal escapes, `\0` in strings, and the
declaration `struct s *p` before `struct s` is complete.  The command sources
that compile (V7: 52 of 80, 32V: 587 of 605) do so with no source change.

## If a decision is made to support some of it

The order of cost and risk, lowest first: **6** (one `tymatch` case), **5**,
**2** (a grammar change with no valid-program risk), **1** (fallback after the
real lookup, so valid programs are unaffected, diagnostics are lost), **4b**, **4a**
(loses the undeclared-identifier diagnostic), **3** (not safe in the general
form; translate the two sources instead).  1 and 2 would clear 23 of the 28 V7
rejections and 10 of the 15 32V language-form rejections.
