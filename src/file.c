#define _XOPEN_SOURCE 700
#include <errno.h>
#include <dirent.h>
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

// https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
static int is_folder(const char* name)
{
    DIR* directory = opendir(name);
    if(directory != NULL) {
        closedir(directory);
        return 1;
    }

    if(errno == ENOTDIR) {
        return 0;
    }

    return -1;
}

static bool is_file_code(const char* path)
{
    size_t path_len = strlen(path);
    if (strncmp(&path[path_len - 2], ".c", 2) == 0) {
        return 1;
    }

    return 0;
}

// https://codeforwin.org/c-programming/c-program-to-list-all-files-in-a-directory-recursively
void file_append_into(char* dir_path, char* dest)
{
    // https://askubuntu.com/questions/859945/what-is-the-maximum-length-of-a-file-path-in-ubuntu
    // Maximum path size should be 4096 bytes and greatest filename
    // should be 255 bytes according to this post
    char path[4096];
    struct dirent *file_entry;
    DIR *dir = opendir(dir_path);
    if (!dir) {
        LOG_ERR("Unable to open '%s' directory\n", dir_path);
        return;
    }

    LOG_INF("looking for code files into '%s'\n", dir_path);
    while ((file_entry = readdir(dir)) != NULL) {
        // Avoid non-sensical path checking
        if (strcmp(file_entry->d_name, ".") == 0 || 
                strcmp(file_entry->d_name, "..") == 0) {
            continue;
        }

        // Build full path
        snprintf(path, sizeof path, "%s/%s",
                dir_path, file_entry->d_name);

        if (is_folder(path) == 1) {
            file_append_into(path, dest);
        } else if (is_file_code(path)) {
            strncat(dest, path, strlen(path) + 1); 
            strncat(dest, " ", 2);
        }
    }

    closedir(dir);
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
