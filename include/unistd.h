/*
 * unistd.h -- V7 libc syscall wrappers for the PDP-11 target.
 * sbrk/brk take int and return char *, per V7's <sys> conventions.
 */
#ifndef _UNISTD_H
#define _UNISTD_H

char	*sbrk(int);
int	brk(char *);

#endif /* _UNISTD_H */
