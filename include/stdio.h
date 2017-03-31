#ifndef __STDIO_H
#define __STDIO_H

#include <compiler.h>
#include <printf.h>
#include <sys/types.h>

void printf(const char *fmt, ...) __PRINTFLIKE(1, 2);
int sprintf(char *str, const char *fmt, ...) __PRINTFLIKE(2, 3);
int snprintf(char *str, size_t len, const char *fmt, ...) __PRINTFLIKE(3, 4);
int vsprintf(char *str, const char *fmt, va_list ap);
int vsnprintf(char *str, size_t len, const char *fmt, va_list ap);

#define PRINTF_SIZE     256

#endif /* __STDIO_H */
