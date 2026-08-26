#define MAX 100
#define SQUARE(x) ((x)*(x))
#if MAX > 50
int big = MAX;
#else
int small = MAX;
#endif
#ifdef unix
int onunix;
#endif
#ifndef pdp11
int notpdp11;
#else
int onpdp11;
#endif
int sq = SQUARE(7);
