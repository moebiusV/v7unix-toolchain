
#include "defs"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static const char shellcom[] = "/bin/sh";

int dosys(char *comstring, int nohalt)
{
register int status;

if(metas(comstring))
	status = doshell(comstring,nohalt);
else	status = doexec(comstring);

return(status);
}



int metas(char *s)   /* Are there are any  Shell meta-characters? */
{
char c;

while( (funny[c = *s++] & META) == 0 )
	;
return( c );
}

int doshell(char *comstring, int nohalt)
{
if((childpid = fork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();

	execl(shellcom, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
	fatal("Couldn't load Shell");
	}

return( await() );
}




int await(void)
{
int status;
register int pid;

enbint(SIG_IGN);
while( (pid = wait(&status)) != childpid)
	if(pid == -1)
		fatal("bad wait code");
childpid = 0;
enbint(intrupt);
return(status);
}




void doclose(void)	/* Close open directory files before exec'ing */
{
struct opendir *od;
for (od = firstod; od; od = od->nxtopendir)
	if (od->dirfc != NULL)
		closedir(od->dirfc);
}




int doexec(char *str)
{
register char *t;
char *argv[200];
register char **p;

while( *str==' ' || *str=='\t' )
	++str;
if( *str == '\0' )
	return(-1);	/* no command */

p = argv;
for(t = str ; *t ; )
	{
	*p++ = t;
	while(*t!=' ' && *t!='\t' && *t!='\0')
		++t;
	if(*t)
		for( *t++ = '\0' ; *t==' ' || *t=='\t'  ; ++t)
			;
	}

*p = NULL;

if((childpid = fork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();
	enbint(intrupt);
	execvp(str, argv);
	fatal1("Cannot load %s",str);
	}

return( await() );
}

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>


void touch(int force, char *name)
{
struct stat stbuff;
char junk[1];
int fd;

if( stat(name,&stbuff) < 0)
	if(force)
		goto create;
	else
		{
		fprintf(stderr, "touch: file %s does not exist.\n", name);
		return;
		}

if(stbuff.st_size == 0)
	goto create;

if( (fd = open(name, 2)) < 0)
	goto bad;

if( read(fd, junk, 1) < 1)
	{
	close(fd);
	goto bad;
	}
lseek(fd, 0L, 0);
if( write(fd, junk, 1) < 1 )
	{
	close(fd);
	goto bad;
	}
close(fd);
return;

bad:
	fprintf(stderr, "Cannot touch %s\n", name);
	return;

create:
	if( (fd = creat(name, 0666)) < 0)
		goto bad;
	close(fd);
}
