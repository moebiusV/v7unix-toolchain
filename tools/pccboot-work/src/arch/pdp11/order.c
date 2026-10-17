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


# include "pass2.h"

#define INCMARK	0x4020	/* n_qual of a UMUL whose address offstar() left to (rN)+ */

#include <string.h>

int canaddr(NODE *);

/* is it legal to make an OREG or NAME entry which has an
 * offset of off, (from a register of r), if the
 * resulting thing had type t */
int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	return(0);  /* YES */
}

/*
 * Is q an address that an autoincrement/decrement operand can express?
 *	(p += n) - n	the old value of p, then p advances by what it points
 *			to:	(rN)+
 *	p -= n		p goes back first:	-(rN)
 * p must be in a register by now, and n the size of the object pointed to,
 * which for these modes is one or two bytes.  Returns 1 for (rN)+, 2 for -(rN).
 */
static int
inctree(NODE *q)
{
	NODE *a, *x;
	CONSZ n;
	TWORD t;
	int mode;

	if (!xdmr)
		return 0;
	if (q->n_op == MINUS && q->n_left->n_op == ASSIGN) {
		a = q->n_left;
		if (a->n_right->n_op != PLUS ||
		    a->n_right->n_right->n_op != ICON ||
		    q->n_right->n_op != ICON ||
		    getlval(a->n_right->n_right) != getlval(q->n_right) ||
		    !treecmp(a->n_left, a->n_right->n_left))
			return 0;
		x = a->n_left;
		n = getlval(q->n_right);
		mode = 1;
	} else if (q->n_op == ASSIGN && q->n_right->n_op == MINUS &&
	    q->n_right->n_right->n_op == ICON &&
	    treecmp(q->n_left, q->n_right->n_left)) {
		x = q->n_left;
		n = getlval(q->n_right->n_right);
		mode = 2;
	} else
		return 0;

	if (x->n_op != REG || !ISPTR(x->n_type))
		return 0;
	t = DECREF(x->n_type);
	/* the step must be the size of what is pointed to: one or two bytes */
	if (n == 1 && (t == CHAR || t == UCHAR))
		return mode;
	if (n == 2 && (ISPTR(t) || t == INT || t == UNSIGNED || t == SHORT ||
	    t == USHORT))
		return mode;
	return 0;
}

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 */
void
offstar(NODE *p, int shape)
{
	NODE *um = NULL;

	if (x2debug)
		printf("offstar(%p)\n", p);

	if (isreg(p))
		return; /* Is already OREG */

	if (p->n_op == UMUL) {
		if (p->n_type < PTR+LONG || p->n_type > PTR+ULONGLONG) {
			um = p;
			p = p->n_left; /* double indexed umul */
		}
	}

	if (inctree(p)) {	/* Do post-inc conversion */
		/*
		 * myormake() must know that the increment has no instruction
		 * of its own: a pointer that only gets its register after the
		 * instructions are chosen is not done this way.
		 */
		if (um != NULL)
			um->n_qual = INCMARK;
		if (offstar_um != NULL)
			offstar_um->n_qual = INCMARK;
		return;
	}

	/*
	 * $name + reg: put the register first and the symbol last, as for
	 * reg + const, so that it becomes the indexed mode name(rN).  The
	 * symbol is made a plain word so that canon() does not move it back
	 * to the left, where pointers go.
	 */
	if (p->n_op == PLUS && p->n_left->n_op == ICON &&
	    p->n_left->n_name[0] != '\0' && p->n_right->n_op != ICON) {
		NODE *t = p->n_left;

		p->n_left = p->n_right;
		p->n_right = t;
		t->n_type = INT;
	}

	if( p->n_op == PLUS || p->n_op == MINUS ){
		if (p->n_right->n_op == ICON) {
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			/* Converted in ormake() */
			return;
		}
	}
	(void)geninsn(p, INAREG);
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 */
void
myormake(NODE *p)
{
	NODE *q = p->n_left;
	int mode;

	if (x2debug) {
		printf("myormake(%p)\n", p);
		fwalk(p, e2print, 0);
	}
	if (p->n_qual == INCMARK && (mode = inctree(q)) != 0) {
		NODE *x = mode == 1 ? q->n_left->n_left : q->n_left;

		/* only bytes and words: a long takes two accesses */
		if ((p->n_type >= LONG && p->n_type <= ULONGLONG) ||
		    x->n_op != REG)
			return;
		p->n_op = OREG;
		setlval(p, 0); /* Add support for index offset */
		p->n_rval = R2PACK(regno(x), 0, mode);
		tfree(q);
		return;
	}
	if (q->n_op != OREG)
		return;
	if (TBLIDX(q->n_su))
		return; /* got sub-conversion, cannot convert */
	if (R2TEST(regno(q)))
		return; /* cannot convert anymore */
	if (p->n_type >= LONG && p->n_type <= ULONGLONG)
		return;
	p->n_op = OREG;
	setlval(p, getlval(q));
	p->n_name = q->n_name;		/* *name(rN): the symbol goes along */
	p->n_rval = R2PACK(q->n_rval, 0, 0);
	if (x2debug) {
		printf("myormake end(%p)\n", p);
		fwalk(p, e2print, 0);
	}
	nfree(q);
}

/*
 * Shape matches for UMUL.  Cooperates with offstar().
 */
int
shumul(NODE *p, int shape)
{
	if (x2debug)
		printf("shumul(%p)\n", p);

	if (p->n_op == NAME && (shape & STARNM))
		return SRDIR;
	if (shape & SOREG)
		return SROREG;	/* Calls offstar */
	return SRNOPE;
}

/*
 * Rewrite operations on binary operators (like +, -, etc...).
 * Called as a result of table lookup.
 */
int
setbin(NODE *p)
{

	if (x2debug)
		printf("setbin(%p)\n", p);
	return 0;

}

/* setup for assignment operator */
int
setasg(NODE *p, int cookie)
{
	if (x2debug)
		printf("setasg(%p)\n", p);
	return(0);
}

/* setup for unary operator */
int
setuni(NODE *p, int cookie)
{
	return(0);
}

/*
 * Set evaluation order of a binary node if it differs from default.
 */
int
setorder(NODE *p)
{
	return 0; /* nothing differs on x86 */
}

/*
 * set registers in calling conventions live.
 */
int *
livecall(NODE *p)
{
	static int r[] = { -1 };

	return r;
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
