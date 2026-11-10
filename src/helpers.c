#include <stdio.h>
#include "main.h"

int path_join(char *out, size_t size, const char *base, const char *name)
{
    int n = snprintf(out, size, "%s/%s", base, name);
    if (n < 0 || (size_t)n >= size) {
        LOG_WRN("path too long: '%s/%s'\n", base, name);
        return -1;
    }
    return 0;
}

void print_banner(void)
{
    fputs("▌     ▗ \n"
          "▙▘▀▌▛▘▜▘\n"
          "▛▖█▌▌ ▐▖\n", stdout);
}
