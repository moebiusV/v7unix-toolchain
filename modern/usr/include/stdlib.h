/*
 * stdlib.h -- V7 libc memory/process functions for the PDP-11 target.
 * V7's malloc/calloc/realloc take `unsigned` (16-bit) sizes.
 */
#ifndef _STDLIB_H
#define _STDLIB_H

char	*malloc(unsigned);
char	*calloc(unsigned, unsigned);
char	*realloc(char *, unsigned);
void	free(char *);
void	exit(int);

#endif /* _STDLIB_H */
