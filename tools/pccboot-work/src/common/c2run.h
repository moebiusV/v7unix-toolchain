/*
 * c2run.h -- run c2 (cc2/cc2.<mach>.c, built into ccom as c2.o with its main
 * renamed c2main) on the assembly ccom has buffered, in place of exec'ing the
 * separate cc2 program.  Included, not linked.
 *
 * c2 is a filter with its own globals and exits when done, so it runs in a
 * forked copy of ccom: the text goes in through a temporary file and comes back
 * through another.  If anything goes wrong the caller writes the text it has,
 * unoptimized but correct.
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern int c2main(int, char **);

/* returns 1 and writes the c2 output to out, or 0 and writes nothing */
static int
c2_filter(const char *text, FILE *out, char *flag)
{
	FILE *in, *res;
	char buf[8192], *av[3];
	size_t n;
	int ac = 0, st;
	pid_t pid;

	if ((in = tmpfile()) == NULL)
		return 0;
	if ((res = tmpfile()) == NULL) {
		fclose(in);
		return 0;
	}
	fputs(text, in);
	fflush(in);
	lseek(fileno(in), 0, SEEK_SET);
	fflush(NULL);
	if ((pid = fork()) < 0) {
		fclose(in);
		fclose(res);
		return 0;
	}
	if (pid == 0) {
		dup2(fileno(in), 0);
		dup2(fileno(res), 1);
		stdin = fdopen(0, "r");
		stdout = fdopen(1, "w");
		av[ac++] = "c2";
		if (flag != NULL)
			av[ac++] = flag;
		av[ac] = NULL;
		c2main(ac, av);
		fflush(stdout);
		_exit(0);
	}
	while (waitpid(pid, &st, 0) < 0)
		;
	fclose(in);
	if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
		fclose(res);
		return 0;
	}
	rewind(res);
	while ((n = fread(buf, 1, sizeof(buf), res)) > 0)
		fwrite(buf, 1, n, out);
	fclose(res);
	return 1;
}
