/*
 * c1 global state.
 *
 * V7 kept these as tentative (common) definitions in c1.h, relying on the
 * linker to merge them across translation units.  C99 has no common symbols, so
 * they live here as the single definition; c1.h declares them `extern`.
 */

#include "c1.h"

char	*funcbase;
char	*curbase;
char	*coremax;
int16_t	nfloat;
int16_t	nstack;
int32_t	totspace;
int16_t	nerror;
int16_t	line;
int16_t	namsiz;
int16_t	xlab1, xlab2, xop, xzero;
struct	node	fczero;
