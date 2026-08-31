/*
 * fcntl.h -- V7 open/creat syscalls for the PDP-11 target (no O_* flags in V7;
 * callers pass 0=read, 1=write).
 */
#ifndef _FCNTL_H
#define _FCNTL_H

int	open(char *, int);
int	creat(char *, int);

#endif /* _FCNTL_H */
