# /* UNIX shell — name-node flags (V7 wrote these as #define). */

enum {
	N_RDONLY = 0100000,
	N_EXPORT = 0040000,
	N_ENVNAM = 0020000,
	N_ENVPOS = 0007777,

	N_DEFAULT = 0,
};

struct namnod {
	NAMPTR	namlft;
	NAMPTR	namrgt;
	STRING	namid;
	STRING	namval;
	STRING	namenv;
	INT	namflg;
};
