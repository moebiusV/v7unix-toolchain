#
/* C compiler
 *
 *
 */

#include "c0.h"
#include <stdint.h>

/*
 * Process a single external definition
 */
int16_t blkend(void);
int16_t blockhead(void);
int16_t cfunc(void);
int16_t cinit(struct node *anp, int16_t flex, int16_t sclass);
int16_t errflush(int16_t ao);
int16_t forstmt(void);
int16_t funchead(void);
struct node * pexpr(void);
void prste(struct node *acs);
int16_t pswitch(void);
int16_t setinit(struct node *anp);
void statement(void);
int16_t strinit(struct node *np, int16_t sclass);

void extdef(void)
{
	register int16_t o;
	int16_t sclass, scflag, *cb;
	struct node typer;
	register struct node *ds;

	if(((o=symbol())==EOFC) || o==SEMI)
		return;
	peeksym = o;
	sclass = 0;
	blklev = 0;
	if (getkeywords(&sclass, &typer)==0) {
		sclass = EXTERN;
		if (peeksym!=NAME)
			goto syntax;
	}
	scflag = 0;
	if (sclass==DEFXTRN) {
		scflag++;
		sclass = EXTERN;
	}
	if (sclass!=EXTERN && sclass!=STATIC && sclass!=TYPEDEF)
		error("Illegal storage class");
	do {
		defsym = 0;
		paraml = 0;
		parame = 0;
		if (sclass==TYPEDEF) {
			decl1(TYPEDEF, &typer, 0, NULL);
			continue;
		}
		decl1(EXTERN, &typer, 0, NULL);
		if ((ds=defsym)==0)
			return;
		funcsym = ds;
		if ((ds->u.tnode.type&XTYPE)==FUNC) {
			if ((peeksym=symbol())==LBRACE || peeksym==KEYW
			 || (peeksym==NAME && csym->u.hshtab.hclass==TYPEDEF)) {
				funcblk.u.tnode.type = decref(ds->u.tnode.type);
				funcblk.u.tnode.strp = ds->u.tnode.strp;
				setinit(ds);
				outcode("BS", SYMDEF, sclass==EXTERN?ds->u.hshtab.name:"");
				cfunc();
				return;
			}
			if (paraml)
				error("Inappropriate parameters");
		} else if ((o=symbol())==COMMA || o==SEMI) {
			peeksym = o;
			o = (length(ds)+ALIGN) & ~ALIGN;
			if (sclass==STATIC) {
				setinit(ds);
				outcode("BSBBSBN", SYMDEF, "", BSS, NLABEL, ds->u.hshtab.name, SSPACE, o);
			} else if (scflag)
				outcode("BSN", CSPACE, ds->u.hshtab.name, o);
		} else {
			if (o!=ASSIGN)
				peeksym = o;
			setinit(ds);
			if (sclass==EXTERN)
				outcode("BS", SYMDEF, ds->u.hshtab.name);
			outcode("BBS", DATA, NLABEL, ds->u.hshtab.name);
			cb = funcbase;
			if (cinit(ds, 1, sclass) & ALIGN)
				outcode("B", EVEN);
			if (maxdecl > cb)
				cb = maxdecl;
			funcbase = cb;
		}
	} while ((o=symbol())==COMMA);
	if (o==SEMI)
		return;
syntax:
	if (o==RBRACE) {
		error("Too many }'s");
		peeksym = 0;
		return;
	}
	error("External definition syntax");
	errflush(o);
	statement();
}

/*
 * Process a function definition.
 */
int16_t cfunc(void)
{
	register int16_t *cb;
	register int16_t sloc;

	sloc = isn;
	isn += 2;
	outcode("BBS", PROG, RLABEL, funcsym->u.hshtab.name);
	if (proflg)
		outcode("BN", PROFIL, isn++);
	cb = curbase;
	regvar = 5;
	autolen = STAUTO;
	maxauto = STAUTO;
	blklev = 1;
	declist(ARG);
	outcode("B", SAVE);
	funchead();
	branch(sloc);
	label(sloc+1);
	retlab = isn++;
	blklev = 0;
	if ((peeksym = symbol()) != LBRACE)
		error("Compound statement required");
	statement();
	outcode("BNB", LABEL, retlab, RETRN);
	label(sloc);
	outcode("BN", SETSTK, -maxauto);
	branch(sloc+1);
	if (cb < maxdecl)
		cb = maxdecl;
	curbase = funcbase = cb;
}

/*
 * Process the initializers for an external definition.
 */
int16_t cinit(struct node *anp, int16_t flex, int16_t sclass)
{
	register struct node *np;
	register int16_t nel, ninit;
	int16_t width, isarray, o, brace, realtype, *cb;
	struct node *s;

	cb = funcbase;
	np = gblock(sizeof(*np));
	funcbase = curbase;
	cpysymb(np, anp);
	realtype = np->u.tnode.type;
	isarray = 0;
	if ((realtype&XTYPE) == ARRAY)
		isarray++;
	else
		flex = 0;
	width = length(np);
	nel = 1;
	/*
	 * If it's an array, find the number of elements.
	 * temporarily modify to look like kind of thing it's
	 * an array of.
	 */
	if (sclass==AUTO)
		if (isarray || realtype==STRUCT)
			error("No auto. aggregate initialization");
	if (isarray) {
		np->u.tnode.type = decref(realtype);
		np->u.tnode.subsp++;
		if (width==0 && flex==0)
			error("0-length row: %.8s", anp->u.hshtab.name);
		o = length(np);
		/* nel = ldiv(0, width, o); */
		nel = (unsigned)width/o;
		width = o;
	}
	brace = 0;
	if ((peeksym=symbol())==LBRACE && (isarray || np->u.tnode.type!=STRUCT)) {
		peeksym = -1;
		brace++;
	}
	ninit = 0;
	do {
		if ((o=symbol())==RBRACE)
			break;
		peeksym = o;
		if (o==STRING && realtype==ARRAY+CHAR) {
			if (sclass==AUTO)
				error("No strings in automatic");
			peeksym = -1;
			putstr(0, flex?10000:nel);
			ninit += nchstr;
			o = symbol();
			break;
		} else if (np->u.tnode.type==STRUCT) {
			strinit(np, sclass);
		} else if ((np->u.tnode.type&ARRAY)==ARRAY || peeksym==LBRACE)
			cinit(np, 0, sclass);
		else {
			initflg++;
			s = tree();
			initflg = 0;
			if (np->u.hshtab.hflag&FFIELD)
				error("No field initialization");
			*cp++ = nblock(np);
			*cp++ = s;
			build(ASSIGN);
			if (sclass==AUTO||sclass==REG)
				rcexpr(*--cp);
			else if (sclass==ENUMCON) {
				if (s->u.tnode.op!=CON)
					error("Illegal enum constant for %.8s", anp->u.hshtab.name);
				anp->u.hshtab.hoffset = s->u.cnode.value;
			} else
				rcexpr(block(INIT,np->u.tnode.type,NULL,NULL,(*--cp)->u.tnode.tr2));
		}
		ninit++;
		if ((ninit&077)==0 && sclass==EXTERN)
			outcode("BS", SYMDEF, "");
	} while ((o=symbol())==COMMA && (ninit<nel || brace || flex));
	if (brace==0 || o!=RBRACE)
		peeksym = o;
	/*
	 * If there are too few initializers, allocate
	 * more storage.
	 * If there are too many initializers, extend
	 * the declared size for benefit of "sizeof"
	 */
	if (ninit<nel && sclass!=AUTO)
		outcode("BN", SSPACE, (nel-ninit)*width);
	else if (ninit>nel) {
		if (flex && nel==0) {
			np->u.tnode.subsp[-1] = ninit;
		} else
			error("Too many initializers: %.8s", anp->u.hshtab.name);
		nel = ninit;
	}
	curbase = funcbase = cb;
	return(nel*width);
}

/*
 * Initialize a structure
 */
int16_t strinit(struct node *np, int16_t sclass)
{
	register struct node **mlp;
	static int16_t zerloc;
	register int16_t o, brace;

	if ((mlp = np->u.tnode.strp->memlist)==NULL) {
		mlp = &zerloc;
		error("Undefined structure initialization");
	}
	brace = 0;
	if ((o = symbol()) == LBRACE)
		brace++;
	else
		peeksym = o;
	do {
		if ((o=symbol()) == RBRACE)
			break;
		peeksym = o;
		if (*mlp==0) {
			error("Too many structure initializers");
			cinit(&funcblk, 0, sclass);
		} else
			cinit(*mlp++, 0, sclass);
		if (*mlp ==  &structhole) {
			outcode("B", EVEN);
			mlp++;
		}
	} while ((o=symbol())==COMMA && (*mlp || brace));
	if (sclass!=AUTO && sclass!=REG) {
		if (*mlp)
			outcode("BN", SSPACE, np->u.tnode.strp->ssize - (*mlp)->u.hshtab.hoffset);
		outcode("B", EVEN);
	}
	if (o!=RBRACE || brace==0)
		peeksym = o;
}

/*
 * Mark already initialized
 */
int16_t setinit(struct node *anp)
{
	register struct node *np;

	np = anp;
	if (np->u.hshtab.hflag&FINIT)
		error("%s multiply defined", np->u.hshtab.name);
	np->u.hshtab.hflag |= FINIT;
}

/*
 * Process one statement in a function.
 */
void statement(void)
{
	register int16_t o, o1, o2;
	int16_t o3;
	struct node *np;
	int16_t sauto, sreg;

stmt:
	switch(o=symbol()) {

	case EOFC:
		error("Unexpected EOF");
	case SEMI:
		return;

	case LBRACE:
		sauto = autolen;
		sreg = regvar;
		blockhead();
		while (!eof) {
			if ((o=symbol())==RBRACE) {
				autolen = sauto;
				if (sreg!=regvar)
					outcode("BN", SETREG, sreg);
				regvar = sreg;
				blkend();
				return;
			}
			peeksym = o;
			statement();
		}
		error("Missing '}'");
		return;

	case KEYW:
		switch(cval) {

		case GOTO:
			if (o1 = simplegoto())
				branch(o1);
			else 
				dogoto();
			goto semi;

		case RETURN:
			doret();
			goto semi;

		case IF:
			np = pexpr();
			o2 = 0;
			if ((o1=symbol())==KEYW) switch (cval) {
			case GOTO:
				if (o2=simplegoto())
					goto simpif;
				cbranch(np, o2=isn++, 0);
				dogoto();
				label(o2);
				goto hardif;

			case RETURN:
				if (nextchar()==';') {
					o2 = retlab;
					goto simpif;
				}
				cbranch(np, o1=isn++, 0);
				doret();
				label(o1);
				o2++;
				goto hardif;

			case BREAK:
				o2 = brklab;
				goto simpif;

			case CONTIN:
				o2 = contlab;
			simpif:
				chconbrk(o2);
				cbranch(np, o2, 1);
			hardif:
				if ((o=symbol())!=SEMI)
					goto syntax;
				if ((o1=symbol())==KEYW && cval==ELSE) 
					goto stmt;
				peeksym = o1;
				return;
			}
			peeksym = o1;
			cbranch(np, o1=isn++, 0);
			statement();
			if ((o=symbol())==KEYW && cval==ELSE) {
				o2 = isn++;
				branch(o2);
				label(o1);
				statement();
				label(o2);
				return;
			}
			peeksym = o;
			label(o1);
			return;

		case WHILE:
			o1 = contlab;
			o2 = brklab;
			label(contlab = isn++);
			cbranch(pexpr(), brklab=isn++, 0);
			statement();
			branch(contlab);
			label(brklab);
			contlab = o1;
			brklab = o2;
			return;

		case BREAK:
			chconbrk(brklab);
			branch(brklab);
			goto semi;

		case CONTIN:
			chconbrk(contlab);
			branch(contlab);
			goto semi;

		case DO:
			o1 = contlab;
			o2 = brklab;
			contlab = isn++;
			brklab = isn++;
			label(o3 = isn++);
			statement();
			label(contlab);
			contlab = o1;
			if ((o=symbol())==KEYW && cval==WHILE) {
				cbranch(tree(), o3, 1);
				label(brklab);
				brklab = o2;
				goto semi;
			}
			goto syntax;

		case CASE:
			o1 = conexp();
			if ((o=symbol())!=COLON)
				goto syntax;
			if (swp==0) {
				error("Case not in switch");
				goto stmt;
			}
			if(swp>=swtab+SWSIZ) {
				error("Switch table overflow");
			} else {
				swp->swlab = isn;
				(swp++)->swval = o1;
				label(isn++);
			}
			goto stmt;

		case SWITCH:
			o1 = brklab;
			brklab = isn++;
			np = pexpr();
			chkw(np, -1);
			rcexpr(block(RFORCE,0,NULL,NULL,np));
			pswitch();
			brklab = o1;
			return;

		case DEFAULT:
			if (swp==0)
				error("Default not in switch");
			if (deflab)
				error("More than 1 'default'");
			if ((o=symbol())!=COLON)
				goto syntax;
			label(deflab = isn++);
			goto stmt;

		case FOR:
			o1 = contlab;
			o2 = brklab;
			contlab = isn++;
			brklab = isn++;
			if (o=forstmt())
				goto syntax;
			label(brklab);
			contlab = o1;
			brklab = o2;
			return;

		case ELSE:
			error("Inappropriate 'else'");
			statement();
			return;
		}
		error("Unknown keyword");
		goto syntax;

	case NAME:
		if (nextchar()==':') {
			peekc = 0;
			struct node *onp = csym;
			if (onp->u.hshtab.hclass>0) {
				if (onp->u.hshtab.hblklev==0) {
					pushdecl(onp);
					onp->u.hshtab.hoffset = 0;
				} else {
					defsym = onp;
					redec();
					goto stmt;
				}
			}
			onp->u.hshtab.hclass = STATIC;
			onp->u.hshtab.htype = ARRAY;
			onp->u.hshtab.hflag |= FLABL;
			if (onp->u.hshtab.hoffset==0)
				onp->u.hshtab.hoffset = isn++;
			label(onp->u.hshtab.hoffset);
			goto stmt;
		}
	}
	peeksym = o;
	rcexpr(tree());

semi:
	if ((o=symbol())==SEMI)
		return;
syntax:
	error("Statement syntax");
	errflush(o);
}

/*
 * Process a for statement.
 */
int16_t forstmt(void)
{
	register int16_t l, o, sline;
	int16_t sline1, *ss;
	struct node *st;

	if ((o=symbol()) != LPARN)
		return(o);
	if ((o=symbol()) != SEMI) {		/* init part */
		peeksym = o;
		rcexpr(tree());
		if ((o=symbol()) != SEMI)
			return(o);
	}
	label(contlab);
	if ((o=symbol()) != SEMI) {		/* test part */
		peeksym = o;
		cbranch(tree(), brklab, 0);
		if ((o=symbol()) != SEMI)
			return(o);
	}
	if ((peeksym=symbol()) == RPARN) {	/* incr part */
		peeksym = -1;
		statement();
		branch(contlab);
		return(0);
	}
	l = contlab;
	contlab = isn++;
	st = tree();
	sline = line;
	if ((o=symbol()) != RPARN)
		return(o);
	ss = funcbase;
	funcbase = curbase;
	statement();
	sline1 = line;
	line = sline;
	label(contlab);
	rcexpr(st);
	line = sline1;
	if (ss < maxdecl)
		ss = maxdecl;
	curbase = funcbase = ss;
	branch(l);
	return(0);
}

/*
 * A parenthesized expression,
 * as after "if".
 */
struct node * pexpr(void)
{
	register int16_t o, t;

	if ((o=symbol())!=LPARN)
		goto syntax;
	t = tree();
	if ((o=symbol())!=RPARN)
		goto syntax;
	return(t);
syntax:
	error("Statement syntax");
	errflush(o);
	return(0);
}

/*
 * The switch statement, which involves collecting the
 * constants and labels for the cases.
 */
int16_t pswitch(void)
{
	register struct swtab *cswp, *sswp;
	int16_t dl, swlab;

	cswp = sswp = swp;
	if (swp==0)
		cswp = swp = swtab;
	branch(swlab=isn++);
	dl = deflab;
	deflab = 0;
	statement();
	branch(brklab);
	label(swlab);
	if (deflab==0)
		deflab = brklab;
	outcode("BNN", SWIT, deflab, line);
	for (; cswp < swp; cswp++)
		outcode("NN", cswp->swlab, cswp->swval);
	outcode("0");
	label(brklab);
	deflab = dl;
	swp = sswp;
}

/*
 * funchead is called at the start of each function
 * to process the arguments, which have been linked in a list.
 * This list is necessary because in
 * f(a, b) float b; int a; ...
 * the names are seen before the types.
 */
/*
 * Structure resembling a block for a register variable.
 */
struct node hreg = { { .hshtab = { REG, 0, 0, NULL, NULL, 0 } } };
struct node areg = { { .tnode = { NAME, 0, NULL, NULL, &hreg } } };
int16_t funchead(void)
{
	register int16_t pl;
	register struct node *cs;
	struct node *bstack[2];

	pl = STARG;
	while(paraml) {
		parame->u.hshtab.next = 0;
		cs = paraml;
		paraml = paraml->u.hshtab.next;
		if (cs->u.hshtab.htype==FLOAT)
			cs->u.hshtab.htype = DOUBLE;
		cs->u.hshtab.hoffset = pl;
		if ((cs->u.hshtab.htype&XTYPE) == ARRAY) {
			cs->u.hshtab.htype -= (ARRAY-PTR);	/* set ptr */
			cs->u.tnode.subsp++;		/* pop dims */
		}
		pl += rlength(cs);
		if (cs->u.hshtab.hclass==AREG && (hreg.u.hshtab.hoffset=goodreg(cs))>=0) {
			bstack[0] = &areg;
			bstack[1] = nblock(cs);
			cp = &bstack[2];
			areg.u.tnode.type = cs->u.hshtab.htype;
			cs->u.hshtab.hclass = AUTO;
			build(ASSIGN);
			rcexpr(bstack[0]);
			cs->u.hshtab.hoffset = hreg.u.hshtab.hoffset;
			cs->u.hshtab.hclass = REG;
		} else
			cs->u.hshtab.hclass = AUTO;
		prste(cs);
	}
	for (cs=hshtab; cs<hshtab+HSHSIZ; cs++) {
		if (cs->u.hshtab.name[0] == '\0')
			continue;
		if (cs->u.hshtab.hclass == ARG || cs->u.hshtab.hclass==AREG)
			error("Not an argument: %.8s", cs->u.hshtab.name);
	}
	outcode("BN", SETREG, regvar);
}

int16_t blockhead(void)
{
	register int16_t r;

	r = regvar;
	blklev++;
	declist(0);
	if (r != regvar)
		outcode("BN", SETREG, regvar);
}

/*
 * After the end of a block, delete local
 * symbols; save those that are external.
 * Also complain about undefined labels.
 */
int16_t blkend(void)
{
	register struct node *cs, *ncs;
	struct node *endcs;
	register int16_t i;

	blklev--;
	for (cs=hshtab; cs->u.hshtab.name[0] && cs<hshtab+HSHSIZ-1; ++cs)
		;
	endcs = cs;
	do  if (cs->u.hshtab.name[0]) {
		if (cs->u.hshtab.hblklev <= blklev)
			continue;
		if ((cs->u.hshtab.hclass!=EXTERN || blklev!=0)
		 && ((cs->u.hshtab.hflag&FLABL)==0 || blklev==0)) {
			if (cs->u.hshtab.hclass==0)
				error("%.8s undefined", cs->u.hshtab.name);
			if ((ncs = cs->u.hshtab.hpdown)==NULL) {
				cs->u.hshtab.name[0] = '\0';
				hshused--;
				cs->u.hshtab.hflag &= FKEYW;
			} else {
				cpysymb(cs, ncs);
			}
			continue;
		}
		/*
		 * Retained name; must rehash.
		 */
		for (i=0; i<NCPS; i++)
			symbuf[i] = cs->u.hshtab.name[i];
		mossym = cs->u.hshtab.hflag&FMOS;
		lookup();
		if ((ncs=csym) != cs) {
			cs->u.hshtab.name[0] = '\0';
			hshused--;
			i = ncs->u.hshtab.hflag;
			cpysymb(ncs, cs);
			ncs->u.hshtab.hflag |= i&FKEYW;
			cs->u.hshtab.hflag &= FKEYW;
		}
		if (ncs->u.hshtab.hblklev>1 || (ncs->u.hshtab.hblklev>0 && ncs->u.hshtab.hclass==EXTERN))
			ncs->u.hshtab.hblklev--;
	} while ((cs = (cs<&hshtab[HSHSIZ-1])? ++cs: hshtab) != endcs);
}

/*
 * write out special definitions of local symbols for
 * benefit of the debugger.  None of these are used
 * by the assembler except to save them.
 */
void prste(struct node *acs)
{
	register struct node *cs;
	register int16_t nkind;

	cs = acs;
	switch (cs->u.hshtab.hclass) {
	case REG:
		nkind = RNAME;
		break;

	case AUTO:
		nkind = ANAME;
		break;

	case STATIC:
		nkind = SNAME;
		break;

	default:
		return;

	}
	outcode("BSN", nkind, cs->u.hshtab.name, cs->u.hshtab.hoffset);
}

/*
 * In case of error, skip to the next
 * statement delimiter.
 */
int16_t errflush(int16_t ao)
{
	register int16_t o;

	o = ao;
	while(o>RBRACE)	/* ; { } */
		o = symbol();
	peeksym  = o;
}
