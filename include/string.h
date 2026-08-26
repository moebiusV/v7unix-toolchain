/*
 * string.h -- V7 libc string functions, declared for the PDP-11 target.
 * V7's count/return types are `int` (16-bit), not size_t.
 */
#ifndef _STRING_H
#define _STRING_H

char	*strcpy(char *, const char *);
char	*strncpy(char *, const char *, int);
char	*strcat(char *, const char *);
char	*strncat(char *, const char *, int);
int	strcmp(const char *, const char *);
int	strncmp(const char *, const char *, int);
int	strlen(const char *);

#endif /* _STRING_H */
