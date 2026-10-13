/* expect: 1 -- copies through register pointers (from V7 ls) */
#define DIRSIZ 14

makename(dir, file)
char *dir, *file;
{
	static char dfile[100];
	register char *dp, *fp;
	register int i;

	dp = dfile;
	fp = dir;
	while (*fp)
		*dp++ = *fp++;
	*dp++ = '/';
	fp = file;
	for (i=0; i<DIRSIZ; i++)
		*dp++ = *fp++;
	*dp = 0;
	return(dfile);
}

int test(void)
{
	char *r;
	static char nm[DIRSIZ + 1] = "cat";
	int bad = 0;

	r = (char *)makename("/bin", nm);
	if (r[0] != '/' || r[1] != 'b' || r[2] != 'i' || r[3] != 'n') bad |= 1;
	if (r[4] != '/' || r[5] != 'c' || r[6] != 'a' || r[7] != 't' || r[8] != 0) bad |= 2;
	if (r[18] != 0) bad |= 4;
	return bad == 0;
}
