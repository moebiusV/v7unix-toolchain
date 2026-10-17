/*
 * x86peep.h -- a text pass over the assembly of a whole compilation unit,
 * shared by the i386 and amd64 backends (-xdmr).  The file is included, not
 * linked: everything is static.
 *
 * (Loop rotation, which used to be here, is done on the IR by looprot() in
 * mip/optim2.c, for every target.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct xp_lines {
	char **v;
	int n;
};

/* a tab-indented conditional jump "jcc target": mnemonic and target */
static int
xp_condjump(const char *s, char *mn, char *tgt)
{
	if (s[0] != '\t' || s[1] != 'j' || strncmp(s + 1, "jmp", 3) == 0)
		return 0;
	return sscanf(s + 1, "%15s %63s", mn, tgt) == 2;
}

/*
 * "testl %r,%r" right after an instruction that left the codes of %r, and
 * before a branch that looks only at those codes, is redundant:
 *	decl %edx; testl %edx,%edx; jl X
 * The ops that clear OF and CF like test does (and, or, xor) serve every
 * branch; add, sub, inc and dec set OF and CF differently, so they serve the
 * branches on Z and S only (and the signed ones, whose overflow case is
 * undefined behaviour in C).
 */
static int
xp_dest(const char *s, const char *ops[], int n, const char **opnd, char *op)
{
	int i;

	if (s[0] != '\t')
		return 0;
	for (i = 0; i < n; i++) {
		size_t l = strlen(ops[i]);

		if (strncmp(s + 1, ops[i], l) == 0 && s[1 + l] == ' ') {
			strcpy(op, ops[i]);
			*opnd = s + 2 + l;
			return 1;
		}
	}
	return 0;
}

static void
xp_deltest(struct xp_lines *L)
{
	static const char *arith[] = { "decl", "incl", "addl", "subl", "decq", "incq", "addq", "subq" };
	static const char *logic[] = { "andl", "orl", "xorl", "andq", "orq", "xorq" };
	int i, o, w;
	char op[8], mn[16], tgt[64], reg[16], t[64];
	const char *opnd, *c;

	for (i = 0; i + 2 < L->n; i++) {
		int islogic;

		if (xp_dest(L->v[i], arith, 8, &opnd, op))
			islogic = 0;
		else if (xp_dest(L->v[i], logic, 6, &opnd, op))
			islogic = 1;
		else
			continue;
		/* the destination register is the last operand */
		c = strrchr(opnd, ',');
		c = c != NULL ? c + 1 : opnd;
		if (c[0] != '%' || strlen(c) > 8 || strchr(c, '(') != NULL)
			continue;
		strcpy(reg, c);
		w = op[3] == 'q' || op[3] == 'l' ? op[3] : 'l';
		snprintf(t, sizeof(t), "\ttest%c %s,%s", w, reg, reg);
		if (strcmp(L->v[i + 1], t) != 0)
			continue;
		if (!xp_condjump(L->v[i + 2], mn, tgt))
			continue;
		/*
		 * The signed relations (jl, jge, ...) look at OF: after an
		 * add/sub/inc/dec that wraps (an unsigned value tested as an
		 * int) they would answer differently from the test.
		 */
		if (!islogic && strcmp(mn, "je") && strcmp(mn, "jne") &&
		    strcmp(mn, "jz") && strcmp(mn, "jnz") && strcmp(mn, "js") &&
		    strcmp(mn, "jns"))
			continue;
		if (islogic && (strcmp(mn, "jp") == 0 || strcmp(mn, "jnp") == 0))
			continue;
		/* drop the test */
		free(L->v[i + 1]);
		for (o = i + 1; o + 1 < L->n; o++)
			L->v[o] = L->v[o + 1];
		L->n--;
	}
}

static void
xp_emit(struct xp_lines *L, FILE *out)
{
	int i;

	for (i = 0; i < L->n; i++)
		fprintf(out, "%s\n", L->v[i]);
}

/* Read the assembly in buf, write it with the redundant tests removed. */
static void
x86_peep(char *buf, FILE *out)
{
	struct xp_lines L;
	char *p, *e;
	int cap = 1024;

	L.v = malloc(cap * sizeof(char *));
	L.n = 0;
	for (p = buf; *p; p = e + 1) {
		e = strchr(p, '\n');
		if (e == NULL)
			e = p + strlen(p);
		if (L.n == cap)
			L.v = realloc(L.v, (cap *= 2) * sizeof(char *));
		L.v[L.n] = malloc(e - p + 1);
		memcpy(L.v[L.n], p, e - p);
		L.v[L.n][e - p] = '\0';
		L.n++;
		if (*e == '\0')
			break;
	}
	xp_deltest(&L);
	xp_emit(&L, out);
	while (L.n > 0)
		free(L.v[--L.n]);
	free(L.v);
}
