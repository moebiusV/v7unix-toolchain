# /* UNIX shell — parser symbols (V7 wrote these as #define). */

/* symbols for parsing */
enum {
	DOSYM = 0405,
	FISYM = 0420,
	EFSYM = 0422,
	ELSYM = 0421,
	INSYM = 0412,
	BRSYM = 0406,
	KTSYM = 0450,
	THSYM = 0444,
	ODSYM = 0441,
	ESSYM = 0442,
	IFSYM = 0436,
	FORSYM = 0435,
	WHSYM = 0433,
	UNSYM = 0427,
	CASYM = 0417,

	SYMREP = 04000,
	ECSYM = (SYMREP|';'),
	ANDFSYM = (SYMREP|'&'),
	ORFSYM = (SYMREP|'|'),
	APPSYM = (SYMREP|'>'),
	DOCSYM = (SYMREP|'<'),
	EOFSYM = 02000,
	SYMFLG = 0400,

	/* arg to `cmd' */
	NLFLG = 1,
	MTFLG = 2,

	/* for peekc */
	MARK = 0100000,

	/* odd chars */
	DQUOTE = '"',
	SQUOTE = '`',
	LITERAL = '\'',
	DOLLAR = '$',
	ESCAPE = '\\',
	BRACE = '{',
};
