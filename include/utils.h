#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <sys/types.h>

int safe_read(int fd, void *buf, size_t count);
int safe_write(int fd, const void *buf, size_t count);

void   *xmalloc(size_t n);
void   *xcalloc(size_t nmemb, size_t size);

unsigned long long now_ms(void);

void info(const char *fmt, ...);
void warn(const char *fmt, ...);
void die (const char *fmt, ...) __attribute__((noreturn));

#endif
