/*
 * Copyright (c) 2016 Leonid Yegoshin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BITS_H
#define _BITS_H

#ifdef __MIPSEB__
#define BITFIELD_FIELD(field, more)                                     \
	field;                                                          \
	more

#elif defined(__MIPSEL__)

#define BITFIELD_FIELD(field, more)                                     \
	more                                                            \
	field;

#else /* !defined (__MIPSEB__) && !defined (__MIPSEL__) */
#error "MIPS but neither __MIPSEL__ nor __MIPSEB__?"
#endif

#define GETBIT(bitmask, bitnum)                                               \
	({  unsigned long _word;                                              \
	    unsigned int _res;                                                \
	    _word = ((unsigned long *)bitmask)[(bitnum)/_MIPS_SZLONG];          \
	    _res = (_word << (_MIPS_SZLONG - 1 - ((bitnum) % _MIPS_SZLONG))) >> \
		    (_MIPS_SZLONG - 1);                                       \
	    _res;                                                             \
	})

#define ffs0(x)                                     \
	({  unsigned long tmp = (x & -x);           \
	    __asm__ __volatile__(                   \
		    "clz    %0,%0"                  \
		    : "=&r"(tmp) :);                \
	    (_MIPS_SZLONG - tmp);                   \
	})

#define clz(x)                                      \
	({  unsigned long tmp;                      \
	    __asm__ __volatile__(                   \
		    "clz    %0,%1"                  \
		    : "=r"(tmp) : "r"(x));          \
	    tmp;                                    \
	})

#endif /* _BITS_H */
