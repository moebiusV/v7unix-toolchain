# /*
#  *	UNIX shell — S. R. Bourne
#  *
#  *	The character-class predicates were macros over two 128-byte tables in
#  *	V7; here they are static inline functions (they all collapse to a
#  *	boolean `&&', so the 0-or-1 result is unchanged).
#  */

/* table 1 */
enum {
	T_SUB = 01, T_MET = 02, T_SPC = 04, T_DIP = 010, T_EOF = 020,
	T_EOR = 040, T_QOT = 0100, T_ESC = 0200,
};

/* table 2 */
enum {
	T_BRC = 01, T_DEF = 02, T_AST = 04, T_DIG = 010, T_FNG = 020,
	T_SHN = 040, T_IDC = 0100, T_SET = 0200,
};

/* for single chars */
enum {
	_TAB = T_SPC, _SPC = T_SPC, _UPC = T_IDC, _LPC = T_IDC, _DIG = T_DIG,
	_EOF = T_EOF, _EOR = T_EOR, _BAR = T_DIP, _HAT = T_MET, _BRA = T_MET,
	_KET = T_MET, _SQB = T_FNG, _AMP = T_DIP, _SEM = T_DIP, _LT = T_DIP,
	_GT = T_DIP, _LQU = (T_QOT|T_ESC), _BSL = T_ESC, _DQU = T_QOT,
	_DOL1 = (T_SUB|T_ESC),
	_CBR = T_BRC, _CKT = T_DEF, _AST = (T_AST|T_FNG), _EQ = T_DEF,
	_MIN = (T_DEF|T_SHN), _PCS = T_SHN, _NUM = T_SHN, _DOL2 = T_SHN,
	_PLS = (T_DEF|T_SET), _AT = T_AST, _QU = (T_DEF|T_FNG|T_SHN),
};

/* abbreviations for tests */
enum { _IDCH = (T_IDC|T_DIG), _META = (T_SPC|T_DIP|T_MET|T_EOR) };

extern unsigned char _ctype1[];
extern unsigned char _ctype2[];

/* nb the original macros took their arg NOT call-by-value; as functions the
 * value is passed in, which is what every call site already does. */

static inline int space(int c)    { return ((c)&QUOTE)==0 && (_ctype1[c]&T_SPC); }
static inline int eofmeta(int c)  { return ((c)&QUOTE)==0 && (_ctype1[c]&(_META|T_EOF)); }
static inline int qotchar(int c)  { return ((c)&QUOTE)==0 && (_ctype1[c]&T_QOT); }
static inline int eolchar(int c)  { return ((c)&QUOTE)==0 && (_ctype1[c]&(T_EOR|T_EOF)); }
static inline int dipchar(int c)  { return ((c)&QUOTE)==0 && (_ctype1[c]&T_DIP); }
static inline int subchar(int c)  { return ((c)&QUOTE)==0 && (_ctype1[c]&(T_SUB|T_QOT)); }
static inline int escchar(int c)  { return ((c)&QUOTE)==0 && (_ctype1[c]&T_ESC); }

static inline int digit(int c)    { return ((c)&QUOTE)==0 && (_ctype2[c]&T_DIG); }
static inline int fngchar(int c)  { return ((c)&QUOTE)==0 && (_ctype2[c]&T_FNG); }
static inline int dolchar(int c)  { return ((c)&QUOTE)==0 && (_ctype2[c]&(T_AST|T_BRC|T_DIG|T_IDC|T_SHN)); }
static inline int defchar(int c)  { return ((c)&QUOTE)==0 && (_ctype2[c]&T_DEF); }
static inline int setchar(int c)  { return ((c)&QUOTE)==0 && (_ctype2[c]&T_SET); }
static inline int digchar(int c)  { return ((c)&QUOTE)==0 && (_ctype2[c]&(T_AST|T_DIG)); }
static inline int letter(int c)   { return ((c)&QUOTE)==0 && (_ctype2[c]&T_IDC); }
static inline int alphanum(int c) { return ((c)&QUOTE)==0 && (_ctype2[c]&_IDCH); }
static inline int astchar(int c)  { return ((c)&QUOTE)==0 && (_ctype2[c]&T_AST); }
