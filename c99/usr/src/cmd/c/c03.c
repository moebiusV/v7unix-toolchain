#
/*
 * C compiler, phase 1
 *
 *
 * Handles processing of declarations,
 * except for top-level processing of
 * externals.
 */

#include "c0.h"
#include <stdint.h>

/*
 * Process a sequence of declaration statements
 */
int16_t align(int16_t type, int16_t offset, int16_t aflen);
int16_t cpysymb(struct node *s1, struct node *s2);
int16_t decl1(int16_t askw, struct node *atptr, int16_t offset, struct node *absname);
int16_t declare(int16_t askw, struct node *tptr, int16_t offset);
int16_t decsyn(int16_t o);
int16_t getkeywords(int16_t *scptr, struct node *tptr);
int16_t getype(struct tdim *adimp, struct node *absname);
int16_t goodreg(struct node *hp);
int16_t pushdecl(struct node *asp);
int16_t redec(void);
struct str * strdec(int16_t mosf, int16_t kind);
int16_t typov(void);

int16_t declist(int16_t sclass)
{
	register int16_t sc, offset;
	struct node typer;

	offset = 0;
	sc = sclass;
	while (getkeywords(&sclass, &typer)) {
		offset = declare(sclass, &typer, offset);
		sclass = sc;
	}
	return(offset+align(INT, offset, 0));
}

/*
 * Read the keywords introducing a declaration statement
 * Store back the storage class, and fill in the type
 * entry, which looks like a hash table entry.
 */
int16_t getkeywords(int16_t *scptr, struct node *tptr)
{
	register int16_t skw, tkw, longf;
	int16_t o, isadecl, ismos, unsignf;

	isadecl = 0;
	longf = 0;
	unsignf = 0;
	tptr->u.hshtab.htype = INT;
	tptr->u.hshtab.hstrp = NULL;
	tptr->u.hshtab.hsubsp = NULL;
	tkw = -1;
	skw = *scptr;
	ismos = skw==MOS||skw==MOU;
	for (;;) {
		mosflg = ismos && isadecl;
		o = symbol();
		if (o==NAME && csym->u.hshtab.hclass==TYPEDEF && tkw<0) {
			tkw = csym->u.hshtab.htype;
			tptr->u.hshtab.hsubsp = csym->u.hshtab.hsubsp;
			tptr->u.hshtab.hstrp = csym->u.hshtab.hstrp;
			isadecl++;
			continue;
		}
		switch (o==KEYW? cval: -1) {
		case AUTO:
		case STATIC:
		case EXTERN:
		case REG:
		case TYPEDEF:
			if (skw && skw!=cval) {
				if (skw==ARG && cval==REG)
					cval = AREG;
				else
					error("Conflict in storage class");
			}
			skw = cval;
			break;
	
		case UNSIGN:
			unsignf++;
			break;

		case LONG:
			longf++;
			break;

		case ENUM:
			strdec(ismos, cval);
			cval = INT;
			goto types;

		case UNION:
		case STRUCT:
			tptr->u.hshtab.hstrp = strdec(ismos, cval);
			cval = STRUCT;
		case INT:
		case CHAR:
		case FLOAT:
		case DOUBLE:
		types:
			if (tkw>=0)
				error("Type clash");
			tkw = cval;
			break;
	
		default:
			peeksym = o;
			if (isadecl==0)
				return(0);
			if (tkw<0)
				tkw = INT;
			if (skw==0)
				skw = blklev==0? DEFXTRN: AUTO;
			if (unsignf) {
				if (tkw==INT)
					tkw = UNSIGN;
				else
					error("Misplaced 'unsigned'");
			}
			if (longf) {
				if (tkw==FLOAT)
					tkw = DOUBLE;
				else if (tkw==INT)
					tkw = LONG;
				else
					error("Misplaced 'long'");
			}
			*scptr = skw;
			tptr->u.hshtab.htype = tkw;
			return(1);
		}
		isadecl++;
	}
}

/*
 * Process a structure, union, or enum declaration; a subroutine
 * of getkeywords.
 */
struct str * strdec(int16_t mosf, int16_t kind)
{
	register int16_t elsize, o;
	register struct node *ssym;
	int16_t savebits;
	struct node **savememlist;
	int16_t savenmems;
	struct str *strp;
	struct node *ds;
	struct node *mems[NMEMS];
	struct node typer;
	int16_t tagkind;

	if (kind!=ENUM) {
		tagkind = STRTAG;
		mosflg = 1;
	} else
		tagkind = ENUMTAG;
	ssym = 0;
	if ((o=symbol())==NAME) {
		ssym = csym;
		mosflg = mosf;
		o = symbol();
		if (o==LBRACE && ssym->u.hshtab.hblklev<blklev)
			pushdecl(ssym);
		if (ssym->u.hshtab.hclass==0) {
			ssym->u.hshtab.hclass = tagkind;
			ssym->u.tnode.strp = gblock(sizeof(*strp));
			funcbase = curbase;
			ssym->u.tnode.strp->ssize = 0;
			ssym->u.tnode.strp->memlist = NULL;
		}
		if (ssym->u.hshtab.hclass != tagkind)
			redec();
		strp = ssym->u.tnode.strp;
	} else {
		strp = gblock(sizeof(*strp));
		funcbase = curbase;
		strp->ssize = 0;
		strp->memlist = NULL;
	}
	mosflg = 0;
	if (o != LBRACE) {
		if (ssym==0)
			goto syntax;
		if (ssym->u.hshtab.hclass!=tagkind)
			error("Bad structure/union/enum name");
		peeksym = o;
	} else {
		ds = defsym;
		mosflg = 0;
		savebits = bitoffs;
		savememlist = memlist;
		savenmems = nmems;
		memlist = mems;
		nmems = 2;
		bitoffs = 0;
		if (kind==ENUM) {
			typer.u.hshtab.htype = INT;
			typer.u.hshtab.hstrp = strp;
			declare(ENUM, &typer, 0);
		} else
			elsize = declist(kind==UNION?MOU:MOS);
		bitoffs = savebits;
		defsym = ds;
		if (strp->ssize)
			error("%.8s redeclared", ssym->u.hshtab.name);
		strp->ssize = elsize;
		*memlist++ = NULL;
		strp->memlist = gblock((memlist-mems)*sizeof(*memlist));
		funcbase = curbase;
		for (o=0; &mems[o] != memlist; o++)
			strp->memlist[o] = mems[o];
		memlist = savememlist;
		nmems = savenmems;
		if ((o = symbol()) != RBRACE)
			goto syntax;
	}
	return(strp);
   syntax:
	decsyn(o);
	return(0);
}

/*
 * Process a comma-separated list of declarators
 */
int16_t declare(int16_t askw, struct node *tptr, int16_t offset)
{
	register int16_t o;
	register int16_t skw, isunion;

	skw = askw;
	isunion = 0;
	if (skw==MOU) {
		skw = MOS;
		isunion++;
		mosflg = 1;
		if ((peeksym=symbol()) == SEMI) {
			o = length(tptr);
			if (o>offset)
				offset = o;
		}
	}
	do {
		if (skw==ENUM && (peeksym=symbol())==RBRACE) {
			o = peeksym;
			peeksym = -1;
			break;
		}
		o = decl1(skw, tptr, isunion?0:offset, NULL);
		if (isunion) {
			o += align(CHAR, o, 0);
			if (o>offset)
				offset = o;
		} else
			offset += o;
	} while ((o=symbol()) == COMMA);
	if (o==RBRACE) {
		peeksym = o;
		o = SEMI;
	}
	if (o!=SEMI && (o!=RPARN || skw!=ARG1))
		decsyn(o);
	return(offset);
}

/*
 * Process a single declarator
 */
int16_t decl1(int16_t askw, struct node *atptr, int16_t offset, struct node *absname)
{
	int16_t t1, chkoff, a, elsize;
	register int16_t skw;
	int16_t type;
	register struct node *dsym;
	register struct node *tptr;
	struct tdim dim;
	struct field *fldp;
	int16_t *dp;
	int16_t isinit;

	skw = askw;
	tptr = atptr;
	chkoff = 0;
	mosflg = skw==MOS;
	dim.rank = 0;
	if (((peeksym=symbol())==SEMI || peeksym==RPARN) && absname==NULL)
		return(0);
	/*
	 * Filler field
	 */
	if (peeksym==COLON && skw==MOS) {
		peeksym = -1;
		t1 = conexp();
		elsize = align(tptr->u.hshtab.htype, offset, t1);
		bitoffs += t1;
		return(elsize);
	}
	t1 = getype(&dim, absname);
	if (t1 == -1)
		return(0);
	if (tptr->u.hshtab.hsubsp) {
		type = tptr->u.hshtab.htype;
		for (a=0; type&XTYPE;) {
			if ((type&XTYPE)==ARRAY)
				dim.dimens[dim.rank++] = tptr->u.hshtab.hsubsp[a++];
			type >>= TYLEN;
		}
	}
	type = tptr->u.hshtab.htype & ~TYPE;
	while (t1&XTYPE) {
		if (type&BIGTYPE) {
			typov();
			type = t1 = 0;
		}
		type = type<<TYLEN | (t1 & XTYPE);
		t1 >>= TYLEN;
	}
	type |= tptr->u.hshtab.htype&TYPE;
	if (absname)
		defsym = absname;
	dsym = defsym;
	if (dsym->u.hshtab.hblklev < blklev)
		pushdecl(dsym);
	if (dim.rank == 0)
		dsym->u.tnode.subsp = NULL;
	else {
		dp = gblock(dim.rank*sizeof(dim.rank));
		funcbase = curbase;
		if (skw==EXTERN)
			maxdecl = curbase;
		for (a=0; a<dim.rank; a++) {
			if ((t1 = dp[a] = dim.dimens[a])
			 && (dsym->u.hshtab.htype&XTYPE) == ARRAY
			 && dsym->u.tnode.subsp[a] && t1!=dsym->u.tnode.subsp[a])
				redec();
		}
		dsym->u.tnode.subsp = dp;
	}
	if ((type&XTYPE) == FUNC) {
		if (skw==AUTO)
			skw = EXTERN;
		if ((skw!=EXTERN && skw!=TYPEDEF) && absname==NULL)
			error("Bad func. storage class");
	}
	if (!(dsym->u.hshtab.hclass==0
	   || ((skw==ARG||skw==AREG) && dsym->u.hshtab.hclass==ARG1)
	   || (skw==EXTERN && dsym->u.hshtab.hclass==EXTERN && dsym->u.hshtab.htype==type)))
		if (skw==MOS && dsym->u.hshtab.hclass==MOS && dsym->u.hshtab.htype==type)
			chkoff = 1;
		else {
			redec();
			goto syntax;
		}
	if (dsym->u.hshtab.hclass && (dsym->u.hshtab.htype&TYPE)==STRUCT && (type&TYPE)==STRUCT)
		if (dsym->u.hshtab.hstrp != tptr->u.hshtab.hstrp) {
			error("Warning: structure redeclaration");
			nerror--;
		}
	dsym->u.hshtab.htype = type;
	if (tptr->u.hshtab.hstrp)
		dsym->u.hshtab.hstrp = tptr->u.hshtab.hstrp;
	if (skw==TYPEDEF) {
		dsym->u.hshtab.hclass = TYPEDEF;
		return(0);
	}
	if (absname)
		return(0);
	if (skw==ARG1) {
		if (paraml==0)
			paraml = dsym;
		else
			parame->u.hshtab.next = dsym;
		parame = dsym;
		dsym->u.hshtab.hclass = skw;
		return(0);
	}
	elsize = 0;
	if (skw==MOS) {
		elsize = length(dsym);
		if ((peeksym = symbol())==COLON) {
			elsize = 0;
			peeksym = -1;
			t1 = conexp();
			a = align(type, offset, t1);
			if (dsym->u.hshtab.hflag&FFIELD) {
				if (((struct field *)dsym->u.hshtab.hstrp)->bitoffs!=bitoffs
			 	 || ((struct field *)dsym->u.hshtab.hstrp)->flen!=t1)
					redec();
			} else {
				dsym->u.hshtab.hstrp = gblock(sizeof(*fldp));
				funcbase = curbase;
			}
			dsym->u.hshtab.hflag |= FFIELD;
			((struct field *)dsym->u.hshtab.hstrp)->bitoffs = bitoffs;
			((struct field *)dsym->u.hshtab.hstrp)->flen = t1;
			bitoffs += t1;
		} else
			a = align(type, offset, 0);
		elsize += a;
		offset += a;
		if (++nmems >= NMEMS) {
			error("Too many structure members");
			nmems -= NMEMS/2;
			memlist -= NMEMS/2;
		}
		if (a)
			*memlist++ = &structhole;
		if (chkoff && dsym->u.hshtab.hoffset != offset)
			redec();
		dsym->u.hshtab.hoffset = offset;
		*memlist++ = dsym;
	}
	if (skw==REG)
		if ((dsym->u.hshtab.hoffset = goodreg(dsym)) < 0)
			skw = AUTO;
	dsym->u.hshtab.hclass = skw;
	isinit = 0;
	if ((a=symbol())!=COMMA && a!=SEMI && a!=RBRACE)
		isinit++;
	if (a!=ASSIGN)
		peeksym = a;
	if (skw==AUTO) {
	/*	if (STAUTO < 0) {	*/
			autolen -= rlength(dsym);
			dsym->u.hshtab.hoffset = autolen;
			if (autolen < maxauto)
				maxauto = autolen;
	/*	} else { 			*/
	/*		dsym->u.hshtab.hoffset = autolen;	*/
	/*		autolen =+ rlength(dsym);	*/
	/*		if (autolen > maxauto)		*/
	/*			maxauto = autolen;	*/
	/*	}			*/
		if (isinit)
			cinit(dsym, 0, AUTO);
	} else if (skw==STATIC) {
		dsym->u.hshtab.hoffset = isn;
		if (isinit) {
			outcode("BBN", DATA, LABEL, isn++);
			if (cinit(dsym, 1, STATIC) & ALIGN)
				outcode("B", EVEN);
		} else
			outcode("BBNBN", BSS, LABEL, isn++, SSPACE, rlength(dsym));
		outcode("B", PROG);
	} else if (skw==REG && isinit)
		cinit(dsym, 0, REG);
	else if (skw==ENUM) {
		if (type!=INT)
			error("Illegal enumeration %.8s", dsym->u.hshtab.name);
		dsym->u.hshtab.hclass = ENUMCON;
		dsym->u.hshtab.hoffset = offset;
		if (isinit)
			cinit(dsym, 0, ENUMCON);
		elsize = dsym->u.hshtab.hoffset-offset+1;
	}
	prste(dsym);
syntax:
	return(elsize);
}

/*
 * Push down an outer-block declaration
 * after redeclaration in an inner block.
 */
int16_t pushdecl(struct node *asp)
{
	register struct node *sp, *nsp;

	sp = asp;
	nsp = gblock(sizeof(*nsp));
	maxdecl = funcbase = curbase;
	cpysymb(nsp, sp);
	sp->u.hshtab.hclass = 0;
	sp->u.hshtab.hflag &= (FKEYW|FMOS);
	sp->u.hshtab.htype = 0;
	sp->u.hshtab.hoffset = 0;
	sp->u.hshtab.hblklev = blklev;
	sp->u.hshtab.hpdown = nsp;
}

/*
 * Copy the non-name part of a symbol
 */
int16_t cpysymb(struct node *s1, struct node *s2)
{
	register struct node *rs1, *rs2;

	rs1 = s1;
	rs2 = s2;
	rs1->u.hshtab.hclass = rs2->u.hshtab.hclass;
	rs1->u.hshtab.hflag = rs2->u.hshtab.hflag;
	rs1->u.hshtab.htype = rs2->u.hshtab.htype;
	rs1->u.hshtab.hoffset = rs2->u.hshtab.hoffset;
	rs1->u.hshtab.hsubsp = rs2->u.hshtab.hsubsp;
	rs1->u.hshtab.hstrp = rs2->u.hshtab.hstrp;
	rs1->u.hshtab.hblklev = rs2->u.hshtab.hblklev;
	rs1->u.hshtab.hpdown = rs2->u.hshtab.hpdown;
}


/*
 * Read a declarator and get the implied type
 */
int16_t getype(struct tdim *adimp, struct node *absname)
{
	static struct node argtype;
	int16_t type;
	register int16_t o;
	register struct node *ds;
	register struct tdim *dimp;

	ds = defsym;
	dimp = adimp;
	type = 0;
	switch(o=symbol()) {

	case TIMES:
		type = getype(dimp, absname);
		if (type==-1)
			return(type);
		if (type&BIGTYPE) {
			typov();
			type = 0;
		}
		return(type<<TYLEN | PTR);

	case LPARN:
		if (absname==NULL || nextchar()!=')') {
			type = getype(dimp, absname);
			if (type==-1)
				return(type);
			ds = defsym;
			if ((o=symbol()) != RPARN)
				goto syntax;
			goto getf;
		}

	default:
		peeksym = o;
		if (absname) {
			defsym = ds = absname;
			absname = NULL;
			goto getf;
		}
		break;

	case NAME:
		defsym = ds = csym;
	getf:
		switch(o=symbol()) {

		case LPARN:
			if (blklev==0) {
				blklev++;
				ds = defsym;
				declare(ARG1, &argtype, 0);
				defsym = ds;
				blklev--;
			} else
				if ((o=symbol()) != RPARN)
					goto syntax;
			if (type&BIGTYPE) {
				typov();
				type = 0;
			}
			type = type<<TYLEN | FUNC;
			goto getf;

		case LBRACK:
			if (dimp->rank>=5) {
				error("Rank too large");
				dimp->rank = 4;
			}
			if ((o=symbol()) != RBRACK) {
				peeksym = o;
				cval = conexp();
				defsym = ds;
				if ((o=symbol())!=RBRACK)
					goto syntax;
			} else {
				if (dimp->rank!=0)
					error("Null dimension");
				cval = 0;
			}
			dimp->dimens[dimp->rank++] = cval;
			if (type&BIGTYPE) {
				typov();
				type = 0;
			}
			type = type<<TYLEN | ARRAY;
			goto getf;
		}
		peeksym = o;
		return(type);
	}
syntax:
	decsyn(o);
	return(-1);
}

/*
 * More bits required for type than allowed.
 */
int16_t typov(void)
{
	error("Type is too complicated");
}

/*
 * Enforce alignment restrictions in structures,
 * including bit-field considerations.
 */
int16_t align(int16_t type, int16_t offset, int16_t aflen)
{
	register int16_t a, t, flen;
	char *ftl;

	flen = aflen;
	a = offset;
	t = type;
	ftl = "Field too long";
	if (flen==0) {
		a += (NBPC+bitoffs-1) / NBPC;
		bitoffs = 0;
	}
	while ((t&XTYPE)==ARRAY)
		t = decref(t);
	if (t!=CHAR) {
		a = (a+ALIGN) & ~ALIGN;
		if (a>offset)
			bitoffs = 0;
	}
	if (flen) {
		if (type==INT || type==UNSIGN) {
			if (flen > NBPW)
				error(ftl);
			if (flen+bitoffs > NBPW) {
				bitoffs = 0;
				a += NCPW;
			}
		} else if (type==CHAR) {
			if (flen > NBPC)
				error(ftl);
			if (flen+bitoffs > NBPC) {
				bitoffs = 0;
				a += 1;
			}
		} else
			error("Bad type for field");
	}
	return(a-offset);
}

/*
 * Complain about syntax error in declaration
 */
int16_t decsyn(int16_t o)
{
	error("Declaration syntax");
	errflush(o);
}

/*
 * Complain about a redeclaration
 */
int16_t redec(void)
{
	error("%.8s redeclared", defsym->u.hshtab.name);
}

/*
 * Determine if a variable is suitable for storage in
 * a register; if so return the register number
 */
int16_t goodreg(struct node *hp)
{
	int16_t type;

	type = hp->u.hshtab.htype;
	/*
	 * Special dispensation for unions
	 */
	if (type==STRUCT && length(hp)<=SZINT)
		type = INT;
	if ((type!=INT && type!=CHAR && type!=UNSIGN && (type&XTYPE)==0)
	 || (type&XTYPE)>PTR || regvar<3)
		return(-1);
	return(--regvar);
}
