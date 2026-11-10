#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include "main.h"

bool file_exists(const char* path)
{
    struct stat st;
    return lstat(path, &st) != 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
int file_write(const char *path, const char *fmt, ...)
{
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        LOG_WRN("could not create file '%s': %s\n", path, strerror(errno));
        return -1;
    }

    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(f, fmt, ap);
    va_end(ap);

    if (n < 0 || fclose(f) != 0) {
        LOG_WRN("could not write file '%s': %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

int file_new(const char *path)
{
    FILE *f = fopen(path, "w+");
    if (f == NULL) {
        LOG_WRN("could not create file '%s': %s\n", path, strerror(errno));
        return -1;
    }

    fclose(f);
    return 0;
}
