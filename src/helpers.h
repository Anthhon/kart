#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdio.h>

#define ELEMENTS_COUNT(vector) (int)(sizeof vector / sizeof vector[0])

int path_join(char *out, size_t size, const char *base, const char *name);
void print_banner(void);

#endif // HELPERS_H_
