#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

ssize_t safe_read(int fd, void *buf, size_t count) {
    size_t off = 0;
    while (off < count) {
        ssize_t n = read(fd, (char*)buf + off, count - off);
        if (n == 0) die("EOF inesperado em pipe");
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) die("read(): %s", strerror(errno));
        off += (size_t)n;
    }
    return (ssize_t)off;
}

ssize_t safe_write(int fd, const void *buf, size_t count) {
    size_t off = 0;
    while (off < count) {
        ssize_t n = write(fd, (char*)buf + off, count - off);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) die("write(): %s", strerror(errno));
        off += (size_t)n;
    }
    return (ssize_t)off;
}

void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) die("malloc falhou (%zu bytes)", n);
    return p;
}
void *xcalloc(size_t nm, size_t sz) {
    void *p = calloc(nm, sz);
    if (!p) die("calloc falhou (%zu×%zu)", nm, sz);
    return p;
}

static unsigned long long t0;
static unsigned long long monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec*1000 + ts.tv_nsec/1000000;
}
unsigned long long now_ms(void) {
    if (!t0) t0 = monotonic_ms();
    return monotonic_ms() - t0;
}

static void vlog(const char *lvl, const char *fmt, va_list ap) {
    fprintf(stderr, "[%6llu ms] %s: ", (unsigned long long)now_ms(), lvl);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}
void info(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vlog("info ", fmt, ap); va_end(ap);
}
void warn(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vlog("WARN ", fmt, ap); va_end(ap);
}
void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vlog("FATAL", fmt, ap); va_end(ap);
    exit(EXIT_FAILURE);
}
