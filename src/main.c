#define _XOPEN_SOURCE 700

#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/tomlc17/tomlc17.h"

#define KART_CONFIG_FILENAME "kart.toml"
#define ELEMENTS_COUNT(vector) (int)(sizeof vector / sizeof vector[0])

#define ANSI_RESET  "\x1b[0m"
#define ANSI_BOLD   "\x1b[1m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_CYAN   "\x1b[36m"
#define ANSI_RED    "\x1b[31m"
#define LOG_WRN(...)  fprintf(stderr, ANSI_BOLD ANSI_YELLOW "[wrn]" ANSI_RESET " kart: " __VA_ARGS__)
#define LOG_INF(...)  fprintf(stdout, ANSI_BOLD ANSI_CYAN   "[inf]" ANSI_RESET " kart: " __VA_ARGS__)
#define LOG_ERR(...)  fprintf(stderr, ANSI_BOLD ANSI_RED    "[err]" ANSI_RESET " kart: " __VA_ARGS__)

#define KART_CONFIG_TEMPLATE              \
    "[package]\n"                         \
    "name = \"%s\"\n"                     \
    "version = \"0.1.0\"\n"               \
    "std = \"c11\"\n"                     \
    "\n"                                  \
    "[build]\n"                           \
    "cflags = [\"-Wall\", \"-Werror\"]\n" \
    "ldflags = []\n"                      \

typedef struct {
    toml_result_t result;
    char* name;
    char* version;
    char* std;
} config_t;

typedef enum {
    // Required fields
    F_NAME, F_VERSION, F_STD,
    
    // TODO: Optional fields
    // TODO: Boolean fields

    F_COUNT,
} field_id;

typedef struct {
    const char* key;
    field_id id;
} field_t;

static const field_t fields[F_COUNT] = {
    [F_NAME]    = {"package.name",    F_NAME},
    [F_VERSION] = {"package.version", F_VERSION},
    [F_STD]     = {"package.std",     F_STD},
};

config_t config = {0};

static void print_banner(void)
{
    fputs("▌     ▗ \n"
          "▙▘▀▌▛▘▜▘\n"
          "▛▖█▌▌ ▐▖\n", stdout);
}

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

static int folder_remove_all(const char *path)
{
    return nftw(path, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}

static int path_join(char *out, size_t size, const char *base, const char *name)
{
    int n = snprintf(out, size, "%s/%s", base, name);
    if (n < 0 || (size_t)n >= size) {
        LOG_WRN("path too long: '%s/%s'\n", base, name);
        return -1;
    }
    return 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static int file_write(const char *path, const char *fmt, ...)
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

static int file_new(const char *path)
{
    FILE *f = fopen(path, "w+");
    if (f == NULL) {
        LOG_WRN("could not create file '%s': %s\n", path, strerror(errno));
        return -1;
    }
    fclose(f);
    return 0;
}

static int folder_new(const char *path)
{
    if (mkdir(path, 0755) != 0) {
        LOG_WRN("could not create folder '%s': %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int cmd_new(const char *name)
{
    static const char *const dirs[]  = { "src", "include", "target", "tests" };
    static const char *const files[] = { "src/main.c", ".gitignore", KART_CONFIG_FILENAME };
    char path[PATH_MAX];

    LOG_INF("creating new project '%s'\n", name);

    if (folder_new(name) != 0)
        return 1;

    // Create folders
    for (size_t i = 0; i < ELEMENTS_COUNT(dirs); i++) {
        if (path_join(path, sizeof path, name, dirs[i]) != 0 ||
            folder_new(path) != 0)
            return 1;
    }

    // Create files
    for (size_t i = 0; i < ELEMENTS_COUNT(files); i++) {
        if (path_join(path, sizeof path, name, files[i]) != 0 ||
            file_new(path) != 0)
            return 1;
    }

    // Set default configuration file
    if (path_join(path, sizeof path, name, KART_CONFIG_FILENAME) != 0 ||
            file_write(path, KART_CONFIG_TEMPLATE, name) != 0) {
        return 1;
    }

    LOG_INF("project created successfully\n");

    return 0;
}

static int cmd_clean(void)
{
    struct stat st;

    if (lstat("target", &st) != 0) {
        if (errno == ENOENT) {
            LOG_WRN("nothing to clean\n");
            return 0;
        }
        LOG_WRN("could not access 'target': %s\n", strerror(errno));
        return 1;
    }

    LOG_INF("removing 'target'\n");
    if (folder_remove_all("target") != 0)
        return 1;

    return 0;
}

// TODO: implement commands
static int cmd_run(void)   { LOG_ERR("run: not implemented\n");   return 1; }
static int cmd_build(void) { LOG_ERR("build: not implemented\n"); return 1; }
static int cmd_test(void)  { LOG_ERR("test: not implemented\n");  return 1; }

#define OPTIONAL_ARGS \
    OPTIONAL_STRING_ARG(operation_new, "", "-n", "name", "Create a new project with a given name")

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(operation_run, "-r", "Run project") \
    BOOLEAN_ARG(operation_build, "-b", "Build project") \
    BOOLEAN_ARG(operation_test, "-t", "Run testing modules") \
    BOOLEAN_ARG(operation_clean, "-c", "Clear project artifacts") \
    BOOLEAN_ARG(enable_hot_reloading, "--hot-reloading", "Enables hot-reloading") \
    BOOLEAN_ARG(help, "-h", "Show help")

#include "easyargs.h"

int main(int argc, char *argv[])
{
    args_t args = make_default_args();

    if (argc == 1 || !parse_args(argc, argv, &args)) {
        print_banner();
        print_help(argv[0]);
        return 1;
    } else if (args.help) {
        print_banner();
        print_help(argv[0]);
        return 0;
    }

    if (args.operation_new[0] != '\0') {
        return cmd_new(args.operation_new);
    } else if (args.operation_clean) {
        return cmd_clean();
    }

    struct stat st; // Safeguard with kart config file
    if (lstat(KART_CONFIG_FILENAME, &st) != 0) {
        if (errno == ENOENT) {
            LOG_WRN("could not find '%s' file\n", KART_CONFIG_FILENAME);
        } else {
            LOG_WRN("could not access '%s' file: %s\n", KART_CONFIG_FILENAME, strerror(errno));
        }
        return 1;
    }

    config.result = toml_parse_file_ex(KART_CONFIG_FILENAME);
    if (!config.result.ok) {
        LOG_WRN("tomlc17: %s\n", config.result.errmsg);
    }

    // Get required fields
    for (int i = 0; i < F_COUNT; i++) {
        toml_datum_t field = toml_seek(config.result.toptab, fields[i].key);
        if (field.type != TOML_STRING) {
            LOG_WRN("missing or invalid '%s' property in config\n", fields[i].key);
        }

        switch (i) {
            case F_NAME:
                config.name = (char*)field.u.s;
                break;
            case F_VERSION:
                config.version = (char*)field.u.s;
                break;
            case F_STD:
                config.std = (char*)field.u.s;
                break;
        }
    }

    if (args.operation_run)
        return cmd_run();
    else if (args.operation_build)
        return cmd_build();
    else if (args.operation_test)
        return cmd_test();

    // Clean-up config content
    toml_free(config.result);
    memset(&config, 0, sizeof(config));

    return 0;
}
