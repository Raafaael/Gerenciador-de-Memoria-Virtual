#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <sys/types.h>

/* I/O robusto ----------------------------------------------------------- */
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t safe_write(int fd, const void *buf, size_t count);

/* Alocação verificada --------------------------------------------------- */
void   *xmalloc(size_t n);
void   *xcalloc(size_t nmemb, size_t size);

/* Cronômetro desde início da simulação ---------------------------------- */
unsigned long long now_ms(void);

/* Logging --------------------------------------------------------------- */
void info(const char *fmt, ...);
void warn(const char *fmt, ...);
void die (const char *fmt, ...) __attribute__((noreturn));

#endif
