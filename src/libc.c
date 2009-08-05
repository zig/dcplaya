#include <stdio.h>


#ifndef NOTYET
/* TEMPORARY !! compatibility for now */
int thd_enabled = 1;

int kos_lazy_cd;
#endif


#if 0
void puts(const char * str)
{
  dbgio_write_str(str);
}
#endif


/*-
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <limits.h>

typedef long long quad_t;
typedef unsigned long long u_quad_t;

#if defined(lint)
typedef unsigned int    qshift_t;
#else
typedef u_quad_t        qshift_t;
#endif



#define H                1
#define L                0

#        define int64_t_C(c)     (c ## LL)
#        define uint64_t_C(c)    (c ## ULL)

#ifndef INT64_MAX
#define INT64_MAX int64_t_C(9223372036854775807)
#endif

#ifndef INT64_MIN
#define INT64_MIN int64_t_C(-9223372036854775808)
#endif

#ifndef UINT64_MAX
#define UINT64_MAX uint64_t_C(0xFFFFFFFFFFFFFFFF)
#endif

#define QUAD_MAX INT64_MAX //0x7fffffffffffffffLL
#define QUAD_MIN INT64_MIN //-0x8000000000000000LL
#define UQUAD_MAX UINT64_MAX //0xffffffffffffffffULL

#define CHAR_BIT 8
#define QUAD_BITS        (sizeof(quad_t) * CHAR_BIT)
#define INT_BITS        (sizeof(int) * CHAR_BIT)
#define HALF_BITS        (sizeof(int) * CHAR_BIT / 2)
#define LONG_BITS        (sizeof(long) * CHAR_BIT)

#define ONE_HALF 0.5f
#define ONE 1.0f

union uu {
        quad_t        q;                /* as a (signed) quad */
        u_quad_t uq;                /* as an unsigned quad */
        int        sl[2];                /* as two signed ints */
        u_int        ul[2];                /* as two unsigned ints */
};

#if 0
NIMP(__ashldi3)
     NIMP(__ashrdi3)
     NIMP(__lshldi3)
     NIMP(__lshrdi3)
     NIMP(__floatdisf)
     NIMP(__fixsfdi)
#else
#if 1
/*
 * Convert (signed) quad to float.
 */
float
__floatdisf(x)
        quad_t x;
{
        float f;
        union uu u;
        int neg;

        /*
         * Get an unsigned number first, by negating if necessary.
         */
        if (x < 0)
                u.q = -x, neg = 1;
        else
                u.q = x, neg = 0;

        /*
         * Now u.ul[H] has the factor of 2^32 (or whatever) and u.ul[L]
         * has the units.  Ideally we could just set f, add INT_BITS to
         * its exponent, and then add the units, but this is portable
         * code and does not know how to get at an exponent.  Machine-
         * specific code may be able to do this more efficiently.
         *
         * Using double here may be excessive paranoia.
         */
        f = (double)u.ul[H] * (((int)1 << (INT_BITS - 2)) * 4.0);
        f += u.ul[L];

	//printf("floatdisf %f\n", (neg ? -f : f));

        return (neg ? -f : f);
}


quad_t
__fixsfdi(float x)
{
        if (x < 0)
                if (x <= QUAD_MIN)
                        return (QUAD_MIN);
                else
                        return ((quad_t)-(u_quad_t)-x);
        else
                if (x >= QUAD_MAX)
                        return (QUAD_MAX);
                else
                        return ((quad_t)(u_quad_t)x);
}


u_quad_t
__fixunssfdi(float f)
{
        double x, toppart;
        union uu t;

        if (f < 0)
                return (UQUAD_MAX);        /* ??? should be 0?  ERANGE??? */
#ifdef notdef                           /* this falls afoul of a GCC bug */
        if (f >= UQUAD_MAX)
                return (UQUAD_MAX);
#else                                   /* so we wire in 2^64-1 instead */
        if (f >= 18446744073709551615.0)        /* XXX */
                return (UQUAD_MAX);
#endif
        x = f;
        /*
         * Get the upper part of the result.  Note that the divide
         * may round up; we want to avoid this if possible, so we
         * subtract `1/2' first.
         */
        toppart = (x - ONE_HALF) / ONE;
        /*
         * Now build a u_quad_t out of the top part.  The difference
         * between x and this is the bottom part (this may introduce
         * a few fuzzy bits, but what the heck).  With any luck this
         * difference will be nonnegative: x should wind up in the
         * range [0..UINT_MAX].  For paranoia, we assume [INT_MIN..
         * 2*UINT_MAX] instead.
         */
        t.ul[H] = (unsigned int)toppart;
        t.ul[L] = 0;
        x -= (double)t.uq;
        if (x < 0) {
                t.ul[H]--;
                x += UINT_MAX;
        }
        if (x > UINT_MAX) {
                t.ul[H]++;
                x -= UINT_MAX;
        }
        t.ul[L] = (u_int)x;
        return (t.uq);
}

/*
 * Shift a (signed) quad value left (arithmetic shift left).
 * This is the same as logical shift left!
 */
quad_t
__ashldi3(a, shift)
        quad_t a;
        qshift_t shift;
{
        union uu aa;

        if (shift == 0)
                return(a);
        aa.q = a;
        if (shift >= INT_BITS) {
                aa.ul[H] = aa.ul[L] << (shift - INT_BITS);
                aa.ul[L] = 0;
        } else {
                aa.ul[H] = (aa.ul[H] << shift) |
                    (aa.ul[L] >> (INT_BITS - shift));
                aa.ul[L] <<= shift;
        }
        return (aa.q);
}


/*
 * Shift an (unsigned) quad value right (logical shift right).
 */
quad_t
__lshrdi3(a, shift)
        quad_t a;
        qshift_t shift;
{
        union uu aa;

        if (shift == 0)
                return(a);
        aa.q = a;
        if (shift >= INT_BITS) {
                aa.ul[L] = aa.ul[H] >> (shift - INT_BITS);
                aa.ul[H] = 0;
        } else {
                aa.ul[L] = (aa.ul[L] >> shift) |
                    (aa.ul[H] << (INT_BITS - shift));
                aa.ul[H] >>= shift;
        }
        return (aa.q);
}

/*
 * Shift an (unsigned) quad value left (logical shift left).
 * This is the same as arithmetic shift left!
 */
quad_t
__lshldi3(a, shift)
        quad_t a;
        qshift_t shift;
{
        union uu aa;

        if (shift == 0)
                return(a);
        aa.q = a;
        if (shift >= INT_BITS) {
                aa.ul[H] = aa.ul[L] << (shift - INT_BITS);
                aa.ul[L] = 0;
        } else {
                aa.ul[H] = (aa.ul[H] << shift) |
                    (aa.ul[L] >> (INT_BITS - shift));
                aa.ul[L] <<= shift;
        }
        return (aa.q);
}


/*
 * Shift a (signed) quad value right (arithmetic shift right).
 */
quad_t
__ashrdi3(a, shift)
        quad_t a;
        qshift_t shift;
{
        union uu aa;

        if (shift == 0)
                return(a);
        aa.q = a;
        if (shift >= INT_BITS) {
                int s;

                /*
                 * Smear bits rightward using the machine's right-shift
                 * method, whether that is sign extension or zero fill,
                 * to get the `sign word' s.  Note that shifting by
                 * INT_BITS is undefined, so we shift (INT_BITS-1),
                 * then 1 more, to get our answer.
                 */
                /* LINTED inherits machine dependency */
                s = (aa.sl[H] >> (INT_BITS - 1)) >> 1;
                /* LINTED inherits machine dependency*/
                aa.ul[L] = aa.sl[H] >> (shift - INT_BITS);
                aa.ul[H] = s;
        } else {
                aa.ul[L] = (aa.ul[L] >> shift) |
                    (aa.ul[H] << (INT_BITS - shift));
                /* LINTED inherits machine dependency */
                aa.sl[H] >>= shift;
        }
        return (aa.q);
}
#endif
#endif

#if 0
/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
#include "errno.h"
#include <ctype.h>
#define ERANGE 5
long
strtol(nptr, endptr, base)
const char *nptr;
char	  **endptr;
int			base;
{
	const char *s = nptr;
	unsigned long acc;
	unsigned char c;
	unsigned long cutoff;
	int			neg = 0,
				any,
				cutlim;

	//printf("strtol '%s' base %d\n", nptr, base);
	/*
	 * Skip white space and pick up leading +/- sign if any. If base is 0,
	 * allow 0x for hex and 0 for octal, else assume decimal; if base is
	 * already 16, allow 0x.
	 */
	do
	{
		c = *s++;
	} while (isspace(c));
	if (c == '-')
	{
		neg = 1;
		c = *s++;
	}
	else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
		c == '0' && (*s == 'x' || *s == 'X'))
	{
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal numbers.
	 * That is the largest legal value, divided by the base.  An input
	 * number that is greater than this value, if followed by a legal
	 * input character, is too big.  One that is equal to this value may
	 * be valid or not; the limit between valid and invalid numbers is
	 * then based on the last digit.  For instance, if the range for longs
	 * is [-2147483648..2147483647] and the input base is 10, cutoff will
	 * be set to 214748364 and cutlim to either 7 (neg==0) or 8 (neg==1),
	 * meaning that if we have accumulated a value > 214748364, or equal
	 * but the next digit is > 7 (or 8), the number is too big, and we
	 * will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long) LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long) base;
	cutoff /= (unsigned long) base;
	for (acc = 0, any = 0;; c = *s++)
	{
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if ((int) c >= base)
			break;
		if (any < 0 || acc > cutoff || acc == cutoff && (int) c > cutlim)
			any = -1;
		else
		{
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0)
	{
		acc = neg ? LONG_MIN : LONG_MAX;
		errno = ERANGE;
	}
	else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = any ? s - 1 : (char *) nptr;
	return acc;
}
#endif


#if 1
/****************************************************************
Copyright (C) 1997 Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/

/* This is for compilers (like MetaWare High C) whose sscanf is broken.
 * It implements only the relevant subset of sscanf.
 * With sensible compilers, you can omit sscanf.o
 * if you add -DSscanf=sscanf to CFLAGS (in the makefile).
 */

#undef L
#ifdef KR_headers
#include "varargs.h"
#else
#include "stddef.h"
#include "stdarg.h"
#include "stdlib.h"
#endif

//#include "stdio1.h"
#include "string.h"

#ifndef Stderr
extern FILE *Stderr;
#endif

#ifdef KR_headers
#ifndef size_t__
#define size_t int
#define size_t__
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

 static void
bad
#ifdef KR_headers
(fmt) char *fmt;
#else
(const char *fmt)
#endif
{
	fprintf(stderr, "bad fmt in Sscanf, starting with \"%s\"\n", fmt);
	//exit(1);
	}

 int
sscanf
#ifdef KR_headers
	(va_alist)
 va_dcl
#else
	(const char *s, const char *fmt, ...)
#endif
{
	char *s0;
	va_list ap;
	long L, * Lp;
	int i, *ip, rc = 0;

#ifdef KR_headers
	char *fmt, *s;
	va_start(ap);
	s = va_arg(ap, char*);
	fmt = va_arg(ap, char*);
#else
	va_start(ap, fmt);
#endif
	for(;;) {
		for(;;) {
			switch(i = *(unsigned char *)fmt++) {
				case 0:
					goto done;
				case '%':
					break;
				default:
					if (i <= ' ') {
						while(*s <= ' ')
							if (!*s++)
								return rc;
						}
					else if (*s++ != i)
						return rc;
					continue;
				}
			break;
			}
		switch(*fmt++) {
			case 'l':
				if (*fmt != 'd')
					bad(fmt);
				fmt++;
				Lp = va_arg(ap, long*);
				L = strtol(s0 = s, &s, 10);
				if (s > s0) {
					rc++;
					*Lp = L;
					continue;
					}
				return rc;
			case 'd':
				ip = va_arg(ap, int*);
				L = strtol(s0 = s, &s, 10);
				if (s > s0) {
					rc++;
					*ip = (int)L;
					continue;
					}
				return rc;
			default:
				bad(fmt);
			}
		}
 done:
	return rc;
	}
#ifdef __cplusplus
	}
#endif

#endif

#include <stdio.h>
#include <arch/spinlock.h>
#include <kos/dbgio.h>
#undef fprintf
static char out[1024];
static spinlock_t mutex = SPINLOCK_INITIALIZER;
int fprintf(FILE * fp, const char *fmt, ...) {
	va_list args;
	int i;

	spinlock_lock(&mutex);
	va_start(args, fmt);
	i = vsprintf(out, fmt, args);
	va_end(args);

	dbgio_write_str(out);
	spinlock_unlock(&mutex);

	return i;
}
