# include "dextern"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

/* The optimizer reuses the parser's storage under its own names: `amem` (the
 * action table, here `a`), `mem0` (production storage), `indgo` (the goto
 * index), `temp1` (temporary vector) and `tystate` (state types).  V7 did
 * this with `#define` aliases; here the real names are used directly. */

int * ggreed = lkst[0].lset;
int * pgo = wsets[0].ws.lset;
int *yypgo = &nontrst[0].tvalue;

int maxspr = 0;  /* maximum spread of any entry */
int maxoff = 0;  /* maximum offset into a array */
int *pmem = mem0;
int *maxa;
static const int nomore = -1000;

int nxdb = 0;
int adb = 0;

int aoutput(void);
int arout(char *s, int *v, int n);
int gin(int i);
int gtnm(void);
int nxti(void);
void osummary(void);
void stin(int i);

int callopt(void){

	register int i, *p, j, k, *q;

	/* read the arrays from tempfile and set parameters */

	if( (finput=fopen(tempname,"r")) == NULL ) error( "optimizer cannot open tempfile" , 0);

	pgo[0] = 0;
	temp1[0] = 0;
	nstate = 0;
	nnonter = 0;
	for(;;){
		switch( gtnm() ){

		case '\n':
			temp1[++nstate] = (--pmem) - mem0;
		case ',':
			continue;

		case '$':
			break;

		default:
			error( "bad tempfile" , 0);
			}
		break;
		}

	temp1[nstate] = yypgo[0] = (--pmem) - mem0;

	for(;;){
		switch( gtnm() ){

		case '\n':
			yypgo[++nnonter]= pmem-mem0;
		case ',':
			continue;

		case EOF:
			break;

		default:
			error( "bad tempfile" , 0);
			}
		break;
		}

	yypgo[nnonter--] = (--pmem) - mem0;



	for( i=0; i<nstate; ++i ){

		k = 32000;
		j = 0;
		q = mem0 + temp1[i+1];
		for( p = mem0 + temp1[i]; p<q ; p += 2 ){
			if( *p > j ) j = *p;
			if( *p < k ) k = *p;
			}
		if( k <= j ){ /* nontrivial situation */
			/* temporarily, kill this for compatibility
			j -= k;  /* j is now the range */
			if( k > maxoff ) maxoff = k;
			}
		tystate[i] = (temp1[i+1]-temp1[i]) + 2*j;
		if( j > maxspr ) maxspr = j;
		}

	/* initialize ggreed table */

	for( i=1; i<=nnonter; ++i ){
		ggreed[i] = 1;
		j = 0;
		/* minimum entry index is always 0 */
		q = mem0 + yypgo[i+1] -1;
		for( p = mem0+yypgo[i]; p<q ; p += 2 ) {
			ggreed[i] += 2;
			if( *p > j ) j = *p;
			}
		ggreed[i] = ggreed[i] + 2*j;
		if( j > maxoff ) maxoff = j;
		}


	/* now, prepare to put the shift actions into the a array */

	for( i=0; i<ACTSIZE; ++i ) amem[i] = 0;
	maxa = amem;

	for( i=0; i<nstate; ++i ) {
		if( tystate[i]==0 && adb>1 ) fprintf( ftable, "State %d: null\n", i );
		indgo[i] = YYFLAG1;
		}

	while( (i = nxti()) != nomore ) {
		if( i >= 0 ) stin(i);
		else gin(-i);

		}

	if( adb>2 ){ /* print a array */
		for( p=amem; p <= maxa; p += 10){
			fprintf( ftable, "%4d  ", p-amem );
			for( i=0; i<10; ++i ) fprintf( ftable, "%4d  ", p[i] );
			fprintf( ftable, "\n" );
			}
		}
	/* write out the output appropriate to the language */

	aoutput();

	osummary();
	unlink(tempname);
	}

int gin(int i){

	register int *p, *r, *s, *q1, *q2;

	/* enter gotos on nonterminal i into array a */

	ggreed[i] = 0;

	q2 = mem0+ yypgo[i+1] - 1;
	q1 = mem0 + yypgo[i];

	/* now, find a place for it */

	for( p=amem; p < &amem[ACTSIZE]; ++p ){
		if( *p ) continue;
		for( r=q1; r<q2; r+=2 ){
			s = p + *r +1;
			if( *s ) goto nextgp;
			if( s > maxa ){
				if( (maxa=s) > &amem[ACTSIZE] ) error( "a array overflow" , 0);
				}
			}
		/* we have found a spot */

		*p = *q2;
		if( p > maxa ){
			if( (maxa=p) > &amem[ACTSIZE] ) error( "a array overflow" , 0);
			}
		for( r=q1; r<q2; r+=2 ){
			s = p + *r + 1;
			*s = r[1];
			}

		pgo[i] = p-amem;
		if( adb>1 ) fprintf( ftable, "Nonterminal %d, entry at %d\n" , i, pgo[i] );
		goto nextgi;

		nextgp:  ;
		}

	error( "cannot place goto %d\n", i );

	nextgi:  ;
	}

void stin(int i){
	register int *r, *s, n, flag, j, *q1, *q2;

	tystate[i] = 0;

	/* enter state i into the a array */

	q2 = mem0+temp1[i+1];
	q1 = mem0+temp1[i];
	/* find an acceptable place */

	for( n= -maxoff; n<ACTSIZE; ++n ){

		flag = 0;
		for( r = q1; r < q2; r += 2 ){
			if( (s = *r + n + amem ) < amem ) goto nextn;
			if( *s == 0 ) ++flag;
			else if( *s != r[1] ) goto nextn;
			}

		/* check that the position equals another only if the states are identical */

		for( j=0; j<nstate; ++j ){
			if( indgo[j] == n ) {
				if( flag ) goto nextn;  /* we have some disagreement */
				if( temp1[j+1] + temp1[i] == temp1[j] + temp1[i+1] ){
					/* states are equal */
					indgo[i] = n;
					if( adb>1 ) fprintf( ftable, "State %d: entry at %d equals state %d\n",
						i, n, j );
					return;
					}
				goto nextn;  /* we have some disagreement */
				}
			}

		for( r = q1; r < q2; r += 2 ){
			if( (s = *r + n + amem ) >= &amem[ACTSIZE] ) error( "out of space in optimizer a array" , 0);
			if( s > maxa ) maxa = s;
			if( *s != 0 && *s != r[1] ) error( "clobber of a array, pos'n %d", s-amem );
			*s = r[1];
			}
		indgo[i] = n;
		if( adb>1 ) fprintf( ftable, "State %d: entry at %d\n", i, indgo[i] );
		return;

		nextn:  ;
		}

	error( "Error; failure to place state %d\n", i );

	}

int nxti(void){ /* finds the next i */
	register int i, max, maxi;

	max = 0;

	for( i=1; i<= nnonter; ++i ) if( ggreed[i] >= max ){
		max = ggreed[i];
		maxi = -i;
		}

	for( i=0; i<nstate; ++i ) if( tystate[i] >= max ){
		max = tystate[i];
		maxi = i;
		}

	if( nxdb ) fprintf( ftable, "nxti = %d, max = %d\n", maxi, max );
	if( max==0 ) return( nomore );
	else return( maxi );
	}

void osummary(void){
	/* write summary */

	register int i, *p;

	if( foutput == NULL ) return;
	i=0;
	for( p=maxa; p>=amem; --p ) {
		if( *p == 0 ) ++i;
		}

	fprintf( foutput, "Optimizer space used: input %d/%d, output %d/%d\n",
		pmem-mem0+1, MEMSIZE, maxa-amem+1, ACTSIZE );
	fprintf( foutput, "%d table entries, %d zero\n", (maxa-amem)+1, i );
	fprintf( foutput, "maximum spread: %d, maximum offset: %d\n", maxspr, maxoff );

	}

int aoutput(void){ /* this version is for C */


	/* write out the optimized parser */

	fprintf( ftable, "# define YYLAST %d\n", maxa-amem+1 );

	arout( "yyact", amem, (maxa-amem)+1 );
	arout( "yypact", indgo, nstate );
	arout( "yypgo", pgo, nnonter+1 );

	}

int arout(char *s, int *v, int n){

	register int i;

	fprintf( ftable, "short %s[]={\n", s );
	for( i=0; i<n; ){
		if( i%10 == 0 ) fprintf( ftable, "\n" );
		fprintf( ftable, "%4d", v[i] );
		if( ++i == n ) fprintf( ftable, " };\n" );
		else fprintf( ftable, "," );
		}
	}


int gtnm(void){

	register int s, val, c;

	/* read and convert an integer from the standard input */
	/* return the terminating character */
	/* blanks, tabs, and newlines are ignored */

	s = 1;
	val = 0;

	while( (c=getc(finput)) != EOF ){
		if( isdigit(c) ){
			val = val * 10 + c - '0';
			}
		else if ( c == '-' ) s = -1;
		else break;
		}

	*pmem++ = s*val;
	if( pmem > &mem0[MEMSIZE] ) error( "out of space" , 0);
	return( c );

	}
