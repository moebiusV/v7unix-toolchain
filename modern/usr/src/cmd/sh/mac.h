# /* UNIX shell — S. R. Bourne.  The ALGOL control-flow dialect (IF/THEN/FI,
#  * WHILE/DO/OD, FOR/DONE, SWITCH/IN/ENDSW, LOCAL/PROC/REG, ANDF/ORF/NEQ,
#  * TRUE/FALSE) was expanded inline in the .c files; what remains are the
#  * single-character constants and MAX. */

enum {
	LOBYTE = 0377,
	STRIP = 0177,
	QUOTE = 0200,

	EOF = 0,
	NL = '\n',
	SP = ' ',
	LQ = '`',
	RQ = '\'',
	MINUS = '-',
	COLON = ':',
};

static inline char *max(char *a, char *b) { return (a) > (b) ? (a) : (b); }
