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

/*
 *      System log/printout module.
 *
 *      It log into buffer and call UART to flush it offline
 */

#include <limits.h>
#include <printf.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <log.h>

static unsigned char buffer[LOGBUFFER_SIZE];
static unsigned buffer_pos;
static unsigned buffer_current_pos;

void uart_start_console(void);

static inline unsigned int di(void)
{  unsigned int reg;
   __asm__ __volatile__("di %0\nehb":"=r"(reg): );
   return reg;
}

static inline unsigned int ei(void)
{  unsigned int reg;
   __asm__ __volatile__("ei %0":"=r"(reg): );
   return reg;
}

#define im_up(ie)       { ie = di(); }
#define im_down(ie)     { if (ie & 1) ei(); }

static int _log_output(const char *string, size_t len)
{
	size_t count = 0;

	unsigned int ie;
	im_up(ie);

	while (count < len) {
		buffer[buffer_pos++] = *string;
		if (buffer_pos >= LOGBUFFER_SIZE)
			buffer_pos = 0;
		string++;
		count++;
	}

	uart_start_console();

	im_down(ie);

	return count;
}

unsigned char *log_char(void)
{
	if (buffer_current_pos == buffer_pos)
		return NULL;
	return &buffer[buffer_current_pos];
}

unsigned char *log_nextchar(void)
{
	unsigned char *pc;
	unsigned int bufpos;

	if (++buffer_current_pos >= LOGBUFFER_SIZE)
		buffer_current_pos = 0;
	if (buffer_current_pos == buffer_pos)
		return NULL;
	pc = &buffer[buffer_current_pos];
	return pc;
}

void _printf(const char *fmt, va_list ap)
{
	char line[PRINTF_SIZE];
	int length;

	length = vsnprintf(line, PRINTF_SIZE, fmt, ap);
	_log_output(line, length);
	return;
}

void printf(const char *fmt, ...)
{
    int err;

    va_list ap;
    va_start(ap, fmt);
    _printf(fmt, ap);
    va_end(ap);

    return;
}

