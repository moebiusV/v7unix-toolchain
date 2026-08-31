#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/types.h>
enum { DIRSIZ = 15 };
#include	<sys/stat.h>
#include	<dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>



/* globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */

 static VOID	addg();


static VOID addg(STRING as1, STRING as2, STRING as3);
int gmatch(STRING s, STRING p);
void makearg(ARGPTR args);

INT expand(STRING as, int rflg)
{
	INT		count;
	DIR		*dirp = 0;
	BOOL		dir=0;
	STRING		rescan = 0;
	register STRING	s, cs;
	ARGPTR		schain = gchain;
	STATBUF		statb;

	if( trapnote&SIGSET ){ return(0); ;}

	s=cs=as;

	/* check for meta chars */
	{
	   register BOOL slash; slash=0;
	   while( !fngchar(*cs)
	   ){	if( *cs++==0
		){	if( rflg && slash ){ break; } else { return(0) ;}
		} else if ( *cs=='/'
		){	slash++;
		;}
	   ;}
	}

	for(;;){	if( cs==s
		){	s=nullstr;
			break;
		} else if ( *--cs == '/'
		){	*cs=0;
			if( s==cs ){ s="/" ;}
			break;
		;}
	}
	if( stat(s,&statb)>=0
	    && S_ISDIR(statb.st_mode)
	    && (dirp=opendir(s))!=0
	){	dir++;
	;}
	count=0;
	if( *cs==0 ){ *cs++=0200 ;}
	if( dir
	){	/* check for rescan */
		register STRING rs; rs=cs;

		do{	if( *rs=='/' ){ rescan=rs; *rs=0; gchain=0 ;}
		}while(	*rs++ );

		{	struct dirent *dp;
			while( (dp=readdir(dirp)) && (trapnote&SIGSET) == 0
			){	if( dp->d_ino==0 ||
				    (*dp->d_name=='.' && *cs!='.')
				){	continue;
				;}
				if( gmatch(dp->d_name, cs)
				){	addg(s,dp->d_name,rescan); count++;
				;}
			;}
		}
		closedir(dirp);

		if( rescan
		){	register ARGPTR	rchain;
			rchain=gchain; gchain=schain;
			if( count
			){	count=0;
				while( rchain
				){	count += expand(rchain->argval,1);
					rchain=rchain->argnxt;
				;}
			;}
			*rescan='/';
		;}
	;}

	{
	   register CHAR	c;
	   s=as;
	   while( c = *s
	   ){	*s++=(c&STRIP?c:'/') ;}
	}
	return(count);
}

int gmatch(STRING s, STRING p)
{
	register INT		scc;
	CHAR		c;

	if( scc = *s++
	){	if( (scc &= STRIP)==0
		){	scc=0200;
		;}
	;}
	switch( c = *p++ ){

	    case '[':
		{BOOL ok; INT lc;
		ok=0; lc=077777;
		while( c = *p++
		){	if( c==']'
			){	return(ok?gmatch(s,p):0);
			} else if ( c==MINUS
			){	if( lc<=scc && scc<=(*p++) ){ ok++ ;}
			} else {	if( scc==(lc=(c&STRIP)) ){ ok++ ;}
			;}
		;}
		return(0);
		}

	    default:
		if( (c&STRIP)!=scc ){ return(0) ;}

	    case '?':
		return(scc?gmatch(s,p):0);

	    case '*':
		if( *p==0 ){ return(1) ;}
		--s;
		while( *s
		){  if( gmatch(s++,p) ){ return(1) ;} ;}
		return(0);

	    case 0:
		return(scc==0);
	}
}

static VOID addg(STRING as1, STRING as2, STRING as3)
{
	register STRING	s1, s2;
	register INT		c;

	s2 = locstak()+BYTESPERWORD;

	s1=as1;
	while( c = *s1++
	){	if( (c &= STRIP)==0
		){	*s2++='/';
			break;
		;}
		*s2++=c;
	;}
	s1=as2;
	while( *s2 = *s1++ ){ s2++ ;}
	if( s1=as3
	){	*s2++='/';
		while( *s2++ = *++s1 );
	;}
	makearg((ARGPTR)endstak(s2));
}

void makearg(ARGPTR args)
{
	args->argnxt=gchain;
	gchain=args;
}

