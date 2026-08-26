/*
 * stdint.h -- C99 fixed-width integer types for the PDP-11 (V7) target.
 * pcc pdp11-bsd (arch/pdp11/macdefs.h): short/int/pointer = 16-bit,
 * long = 32-bit.
 */
#ifndef _STDINT_H
#define _STDINT_H

typedef short		int16_t;
typedef unsigned short	uint16_t;
typedef long		int32_t;
typedef unsigned long	uint32_t;

typedef short		int_least16_t;
typedef unsigned short	uint_least16_t;
typedef long		int_least32_t;
typedef unsigned long	uint_least32_t;

typedef int		intptr_t;
typedef unsigned int	uintptr_t;

#endif /* _STDINT_H */
