#include <linux/limits.h>
#define _XOPEN_SOURCE 700
#include <string.h>
#include <ftw.h>
#include <errno.h>

#include "main.h"
#include "folder.h"

static int remove_entry(const char *path, const struct stat *sb, int type, struct FTW *ftw)
{
    // Cast to generic types
    (void)sb; (void)type; (void)ftw;

    if (remove(path) != 0) {
        LOG_WRN("could not remove '%s': %s\n", path, strerror(errno));
        return -1; // Nonzero stops walk
    }
    return 0;
}


int folder_new(const char *path)
{
    if (mkdir(path, 0755) != 0) {
        LOG_WRN("could not create folder '%s': %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

int folder_remove_all(const char *path)
{
    return nftw(path, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}
