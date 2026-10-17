/*	$Id$	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c2run.h"
# include "pass1.h"

#ifndef LANG_CXX
#define NODE P1ND
#undef NIL
#define NIL NULL
#define	talloc p1alloc
#define	sap sss
#define	n_type ptype
#undef	n_ap
#define	n_ap pss
#undef	n_df
#define	n_df pdf
#endif

/*
 * Print out assembler segment name.
 */
void
setseg(int seg, char *name)
{
	switch (seg) {
	case PROG:
		/* defloc() prints ".globl _f" first, then ".text", like c1 */
		return;
	case STRNG:
	case RDATA:
	case DATA:
	case LDATA: name = ".data"; break;
	case UDATA: break;
	default:
		cerror("setseg");
	}
	printf("\t%s\n", name);
}

void
defalign(int al)
{
	/* functions are always even; c1 emits no .even in text */
	if (al > ALCHAR && lastloc != PROG)
		printf(".even\n");
}

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	static char *loctbl[] = { "text", "data", "data" };
	TWORD t;
	char *n;
	int s;

	t = sp->stype;
	n = getexname(sp);
	if (ISFTN(t)) {
		/* c1 order: ".globl _f", ".text", then the labels */
		if (sp->sclass == EXTDEF)
			printf(".globl\t%s\n", n);
		printf(".text\n");
		lastloc = PROG;
		printf("%s:\n", n);
		printf("~~%s:\n", n+1);
		return;
	}
	s = ISCON(cqual(t, sp->squal)) ? RDATA : DATA;
	if (s != lastloc)
		printf("	.%s\n", loctbl[s]);
	lastloc = s;
	if (sp->sclass == EXTDEF)
		printf("	.globl %s\n", n);
	if (sp->slevel == 0) {
		printf("%s:\n", n);
		printf("~~%s:\n", n+1);
	} else {
		printf(LABFMT ":\n", sp->soffset);
	}
}

/*
 * Code for the end of a function. Deals with struct return here.
 * On pdp11 we use a static bounce-buffer for struct return, 
 * which address is returned in r0.
 */
void
efcode(void)
{
	struct symtab *sp;
	NODE *p, *q;

	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;

	/* Handcraft a static buffer */
	sp = getsymtab("007", SSTMT);
	sp->stype = DECREF(cftnsp->stype);
	sp->sdf = cftnsp->sdf;
	sp->sap = cftnsp->sap;
	sp->sclass = STATIC;
	sp->soffset = getlab();
	sp->slevel = 1;
	defzero(sp);

	/* create struct assignment */
	q = nametree(sp);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);

	/* return address of buffer */
	q = buildtree(ADDROF, nametree(sp), NIL);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **sp, int cnt)
{
	struct symtab *sp2;
	NODE *n;
	int i;

#ifdef REGVARS
	/*
	 * Parameters declared 'register' are copied into their register
	 * (r4, r3, r2) from the stack slot the caller filled in.
	 */
	for (i = 0; i < cnt; i++) {
		struct symtab slot;

		if (!regparam_is(sp[i]->sname))
			continue;
		slot = *sp[i];			/* still the stack parameter */
		if (!regvar_take(sp[i]))
			continue;
		n = buildtree(ASSIGN, nametree(sp[i]), nametree(&slot));
		ecomp(n);
	}
#endif

	if (xtemps == 0)
		return;

	/* put arguments in temporaries */
	for (i = 0; i < cnt; i++) {
		if (sp[i]->sflags & SINREG)
			continue;
		if (sp[i]->stype == STRTY || sp[i]->stype == UNIONTY ||
		    cisreg(sp[i]->stype) == 0)
			continue;
		sp2 = sp[i];
		n = tempnode(0, sp[i]->stype, sp[i]->sdf, sp[i]->sap);
		n = buildtree(ASSIGN, n, nametree(sp2));
		sp[i]->soffset = regno(n->n_left);
		sp[i]->sflags |= STNODE;
		ecomp(n);
	}
}


void flushbss(void);
void puthelpers(void);	/* local.c */
extern int xnoframeless;	/* -xnoframeless: always use csv/cret */

/*
 * The assembly goes through memory so that each function can be looked at
 * whole before it is written: see frameless().
 */
static FILE *realout;
static char *obuf;
static size_t olen;

static int
isword(const char *s, const char *w)
{
	size_t n = strlen(w);
	const char *q;

	for (q = s; (q = strstr(q, w)) != NULL; q++) {
		if ((q == s || !(isalnum((unsigned char)q[-1]) || q[-1] == '_')) &&
		    !(isalnum((unsigned char)q[n]) || q[n] == '_'))
			return 1;
	}
	return 0;
}

/*
 * Is every use of r5 in this line a positive "N(r5)" argument reference?
 * If so rewrite them for a frameless function (no csv: the return address
 * is at 0(sp), so the first argument is at 2(sp) and not 4(r5)).  With out
 * NULL only check.
 */
static int
fixr5(const char *line, char *out)
{
	const char *p = line;
	char *o = out;
	int ok = 1;

	while (*p) {
		if (p[0] == 'r' && p[1] == '5' &&
		    !(isalnum((unsigned char)p[2]) || p[2] == '_') &&
		    (p == line || !(isalnum((unsigned char)p[-1]) ||
		    p[-1] == '_'))) {
			/* must be "(r5)" with octal digits before the "(" */
			const char *b = p - 1;
			char *d;
			int v, nd = 0;

			if (p == line || p[-1] != '(' || p[2] != ')')
				return 0;
			b = p - 2;
			while (b >= line && *b >= '0' && *b <= '7')
				b--, nd++;
			if (nd == 0 || (b >= line && *b == '-'))
				return 0;
			v = (int)strtol(b + 1, NULL, 8);
			if (v < 4)
				return 0;
			if (out != NULL) {
				o -= nd + 1;		/* drop "N(" already copied */
				d = o;
				o += sprintf(d, "%o(sp", v - 2);
				p += 2;			/* skip "r5", keep ")" */
				continue;
			}
			p += 2;
			continue;
		}
		if (out != NULL)
			*o++ = *p;
		p++;
	}
	if (out != NULL)
		*o = '\0';
	return ok;
}

/*
 * A function that makes no calls, keeps nothing on the stack and uses no
 * register above r1 has no need of csv and cret (4 and 4 bytes): it returns
 * with "rts pc" and reads its arguments from sp.  Written to out.
 */
static void
frameless(char *buf, FILE *out)
{
	char **ln = NULL, *p, *e;
	size_t n = 0, cap = 0, i, j, k;
	static const char *bad[] = { "jsr", "sp", "r2", "r3", "r4", "pc", NULL };

	for (p = buf; *p; p = e) {
		if ((e = strchr(p, '\n')) == NULL)
			e = p + strlen(p);
		if (n == cap)
			ln = realloc(ln, (cap = cap ? cap * 2 : 1024) * sizeof(char *));
		ln[n] = malloc(e - p + 1);
		memcpy(ln[n], p, e - p);
		ln[n][e - p] = '\0';
		n++;
		if (*e == '\n')
			e++;
	}
	for (i = 0; i < n; i++) {
		if (strcmp(ln[i], "jsr\tr5,csv") != 0) {
			fprintf(out, "%s\n", ln[i]);
			continue;
		}
		/* find the end of this function */
		for (j = i + 1; j < n; j++) {
			size_t l = strlen(ln[j]);

			if (strcmp(ln[j], "jsr\tr5,csv") == 0)
				break;
			if (l >= 8 && strcmp(ln[j] + l - 8, "jmp\tcret") == 0)
				break;
			if (ln[j][0] == '.' && strncmp(ln[j], ".=", 2) != 0)
				break;		/* directives: past the code */
		}
		if (j >= n || strcmp(ln[j] + strlen(ln[j]) - 8, "jmp\tcret") != 0)
			goto keep;
		for (k = i + 1; k < j; k++) {
			int b;

			for (b = 0; bad[b] != NULL; b++)
				if (isword(ln[k], bad[b]))
					goto keep;
			if (strstr(ln[k], "cret") != NULL ||
			    strstr(ln[k], "csv") != NULL || !fixr5(ln[k], NULL))
				goto keep;
		}
		/* frameless: no csv, arguments off sp, rts at the end */
		for (k = i + 1; k < j; k++) {
			char t[512];

			if (strlen(ln[k]) > 200) {
				/* too odd to rewrite: put the csv back */
				goto keep;
			}
			fixr5(ln[k], t);
			fprintf(out, "%s\n", t);
		}
		{
			size_t l = strlen(ln[j]);

			ln[j][l - 8] = '\0';
			fprintf(out, "%srts\tpc\n", ln[j]);
		}
		i = j;
		continue;
	keep:
		fprintf(out, "%s\n", ln[i]);
	}
	for (i = 0; i < n; i++)
		free(ln[i]);
	free(ln);
}

/*
 * "mov x,x" copies nothing (the register allocator leaves them when it
 * could not coalesce a move).  It does set the condition codes, so it is
 * only dropped when the next instruction is one that does not read them.
 */
static int
flagblind(const char *s)
{
	static const char *ok[] = { "mov", "movb", "add", "sub", "cmp", "cmpb",
	    "tst", "tstb", "clr", "clrb", "inc", "dec", "jsr", "bis", "bic",
	    "bit", "mul", "div", "ash", "ashc", "neg", "com", "asl", "asr",
	    "jmp", "jbr", "rts", "movf", "movif", "movfi", NULL };
	size_t n = strcspn(s, "\t ");
	int i;

	for (i = 0; ok[i] != NULL; i++)
		if (strlen(ok[i]) == n && strncmp(s, ok[i], n) == 0)
			return 1;
	return 0;
}

static int
selfmove(const char *s)
{
	const char *c, *nl;
	size_t n;

	if (strncmp(s, "mov\t", 4) != 0)
		return 0;
	s += 4;
	nl = strchr(s, '\n');
	if ((c = strchr(s, ',')) == NULL || (nl != NULL && c > nl))
		return 0;
	n = c - s;
	if (nl == NULL || (size_t)(nl - (c + 1)) != n ||
	    strncmp(s, c + 1, n) != 0)
		return 0;
	/* (rN)+ and -(rN) change the register each time they are used */
	if (memchr(s, ')', n) != NULL && s[n-1] == '+')
		return 0;
	if (s[0] == '-' && s[1] == '(')
		return 0;
	return 1;
}

static void
dropselfmoves(char *buf)
{
	char *p, *e, *nx, *q;

	for (p = buf; *p; p = e) {
		if ((e = strchr(p, '\n')) == NULL)
			e = p + strlen(p);
		else
			e++;
		if (!selfmove(p) || *e == '\0')
			continue;
		nx = e;
		/* skip labels standing alone: whatever follows them decides */
		while (nx[0] == 'L' && isdigit((unsigned char)nx[1])) {
			char *c = nx + 1;

			while (isdigit((unsigned char)*c))
				c++;
			if (c[0] != ':' || c[1] != '\n')
				break;
			nx = c + 2;
		}
		/* a label may share its line with the instruction */
		if (nx[0] == 'L' && isdigit((unsigned char)nx[1])) {
			char *c = nx + 1;

			while (isdigit((unsigned char)*c))
				c++;
			if (c[0] == ':')
				nx = c + 1;
		}
		if (!flagblind(nx))
			continue;
		/* blank the instruction out, keeping the line structure */
		for (q = p; q < e - 1; q++)
			*q = ' ';
		memmove(p, e, strlen(e) + 1);
		e = p;
	}
}

void
bjobcode(void)
{
	extern char *asspace;

	/* ".word" is not printed out for pdp11 as */
	astypnames[INT] = astypnames[UNSIGNED] = "";
	asspace = ".=.+"; /* advance counter, not .space */

	if ((xnoframeless || !xdmr) && !xc2)
		return;		/* no buffering: debug output stays in order */
	realout = stdout;
	if ((stdout = open_memstream(&obuf, &olen)) == NULL)
		stdout = realout, realout = NULL;
}

/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag)
{
	/* c1 ends every file with an empty ".globl" and a ".data" */
	if (!flag) {
		puthelpers();
		flushbss();
		printf(".globl\n.data\n");
	}
	if (realout != NULL) {
		FILE *dst = realout, *tm = NULL;
		char *tbuf = NULL;
		size_t tlen = 0;

		fclose(stdout);
		stdout = realout;
		if (!flag) {
			/* with -xc2 the text goes through c2 before it is written */
			if (xc2 && (tm = open_memstream(&tbuf, &tlen)) != NULL)
				dst = tm;
			if (!xdmr || xnoframeless)
				fputs(obuf, dst);
			else {
				dropselfmoves(obuf);
				frameless(obuf, dst);
			}
			if (tm != NULL) {
				fclose(tm);
				/* "=": V7 c2 mis-tracks constants through inc/dec */
				if (!c2_filter(tbuf, stdout, "="))
					fputs(tbuf, stdout);
				free(tbuf);
			}
		}
		free(obuf);
	}
}


/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */
NODE *
funcode(NODE *p)
{
	NODE *r, *l;

	/* Fix function call arguments. On x86, just add funarg */
	for (r = p->n_right; r->n_op == CM; r = r->n_left) {
		if (r->n_right->n_op != STARG)
			r->n_right = block(FUNARG, r->n_right, NIL,
			    r->n_right->n_type, r->n_right->n_df,
			    r->n_right->n_ap);
	}
	if (r->n_op != STARG) {
		l = talloc();
		*l = *r;
		r->n_op = FUNARG;
		r->n_left = l;
		r->n_type = l->n_type;
	}
	return p;
}

/* fix up type of field p */
void
fldty(struct symtab *p)
{
}

/*
 * XXX - fix genswitch.
 */
int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}

/*
 * Return "canonical frame address".
 */
NODE *
builtin_cfa(const struct bitable *bt, NODE *a)
{
	uerror(__func__);
	return bcon(0);
}

NODE *
builtin_return_address(const struct bitable *bt, NODE *a)
{
	uerror(__func__);
	return bcon(0);
}

NODE *
builtin_frame_address(const struct bitable *bt, NODE *a)
{
	uerror(__func__);
	return bcon(0);
}
