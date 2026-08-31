# /*
#  *	UNIX shell — S. R. Bourne
#  *
#  *	The stack workspace helpers were macros in V7; here they are static
#  *	inline functions.  To use the stack as temporary workspace across a
#  *	possible storage allocation: (a) get an offset with `relstak', (b) use
#  *	`pushstak', (c) reset with `setstak', (d) `absstak' gives the real
#  *	address if needed.
#  */

/* a chain of ptrs of stack blocks covered by heap allocation; `tdystak'
 * returns them to the heap */
extern BLKPTR stakbsy;

extern STKPTR stakbas;	/* base of the entire stack */
extern STKPTR brkend;	/* top of entire stack */
extern STKPTR stakbot;	/* base of current item */
extern STKPTR staktop;	/* top of current item */

/* for local use only since it hands out a real address for the stack top */
extern STKPTR locstak();

/* for use after `locstak' to hand back new stack top then allocate item;
 * returns raw memory so it is void* (cast at the caller) */
extern void *endstak();

/* copy a string onto the stack and allocate the space */
extern STKPTR cpystak();

/* allocate given amount of stack space; returns raw memory, cast at caller */
extern void *getstak();

/* used with tdystak */
extern STKPTR savstak();

static inline int relstak(void)       { return (int)(staktop - stakbot); }
static inline STKPTR absstak(int x)   { return stakbot + x; }
static inline void setstak(int x)     { staktop = absstak(x); }
static inline int pushstak(int c)     { return *staktop++ = (char)c; }
static inline int zerostak(void)      { return *staktop = 0; }

/* used to address an item left on top of the stack (very temporary) */
static inline STKPTR curstak(void)    { return staktop; }

/* `usestak' before `pushstak' then `fixstak' — safe against heap allocation */
static inline void usestak(void)      { locstak(); }

/* will allocate the item being used and return its address (safe now) */
static inline STKPTR fixstak(void)    { return endstak(staktop); }
