/*
 * Header for object code improver
 */

#include <stdio.h>
#include <stdint.h>

#define	JBR	1
#define	CBR	2
#define	JMP	3
#define	LABEL	4
#define	DLABEL	5
#define	EROU	7
#define	JSW	9
#define	MOV	10
#define	CLR	11
#define	COM	12
#define	INC	13
#define	DEC	14
#define	NEG	15
#define	TST	16
#define	ASR	17
#define	ASL	18
#define	SXT	19
#define	CMP	20
#define	ADD	21
#define	SUB	22
#define	BIT	23
#define	BIC	24
#define	BIS	25
#define	MUL	26
#define	DIV	27
#define	ASH	28
#define	XOR	29
#define	TEXT	30
#define	DATA	31
#define	BSS	32
#define	EVEN	33
#define	MOVF	34
#define	MOVOF	35
#define	MOVFO	36
#define	ADDF	37
#define	SUBF	38
#define	DIVF	39
#define	MULF	40
#define	CLRF	41
#define	CMPF	42
#define	NEGF	43
#define	TSTF	44
#define	CFCC	45
#define	SOB	46
#define	JSR	47
#define	END	48

#define	JEQ	0
#define	JNE	1
#define	JLE	2
#define	JGE	3
#define	JLT	4
#define	JGT	5
#define	JLO	6
#define	JHI	7
#define	JLOS	8
#define	JHIS	9

#define	BYTE	100
#define	LSIZE	512

struct node {
	char	op;
	char	subop;
	struct	node	*forw;
	struct	node	*back;
	struct	node	*ref;
	int16_t	labno;
	char	*code;
	int16_t	refc;
};

struct optab {
	char	*opstring;
	int16_t	opcode;
} optab[];

char	line[LSIZE];
struct	node	first;
char	*curlp;
int16_t	nbrbr;
int16_t	nsaddr;
int16_t	redunm;
int16_t	iaftbr;
int16_t	njp1;
int16_t	nrlab;
int16_t	nxjump;
int16_t	ncmot;
int16_t	nrevbr;
int16_t	loopiv;
int16_t	nredunj;
int16_t	nskip;
int16_t	ncomj;
int16_t	nsob;
int16_t	nrtst;
int16_t	nlit;

int16_t	nchange;
int16_t	isn;
int16_t	debug;
int16_t	lastseg;
char	*lasta;
char	*lastr;
char	*firstr;
char	revbr[];
char	regs[12][20];
char	conloc[20];
char	conval[20];
char	ccloc[20];

#define	RT1	10
#define	RT2	11
#define	FREG	5
#define	NREG	5
#define	LABHS	127
#define	OPHS	57

struct optab *ophash[OPHS];
struct	node *nonlab();
char	*copy();
char	*sbrk();
char	*findcon();
struct	node *insertl();
struct	node *codemove();
char	*sbrk();
char	*alloc();
