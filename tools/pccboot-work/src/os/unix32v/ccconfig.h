/*	$Id$	*/

/*
 * Copyright (c) 2004 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * 32V: the VAX port of Seventh Edition Unix.  No ELF, no GNU as: the
 * assembler is the V7 "as" (grammar in 32V's cmd/as), the C library is
 * /lib/libc.a and the start-up file /lib/crt0.o.  The VAX backend writes
 * that assembler's dialect when it is built for this target (see
 * VAX_AS_32V in arch/vax/macdefs.h).
 */

/* common cpp predefines */
#define CPPADD	{ "-Dunix", NULL }

#define CRT0		"/lib/crt0.o"
#define CRTBEGIN	0
#define CRTEND		0
#define CRTI		0
#define CRTN		0

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

#if defined(mach_vax)
#define	CPPMDADD { "-D__vax__", "-Dvax", NULL, }
#define DEFLIBS { "-lc", 0 }
#else
#error 32V is a VAX system
#endif

/*
 * -O runs 32V's c2 (cc2.vax.c, built as vax-unknown-unix32v-cc2) over
 * ccom's output, as 32V's cc -O did.
 */
#define	HAVE_CC2

#define	NO_AS_V
#define NO_LD_D
