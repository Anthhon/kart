#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define KART_LOG(...) fprintf(stderr, "kart: " __VA_ARGS__)

static void print_banner(void)
{
    fputs("▌     ▗ \n"
          "▙▘▀▌▛▘▜▘\n"
          "▛▖█▌▌ ▐▖\n", stdout);
}

static int path_join(char *out, size_t size, const char *base, const char *name)
{
    int n = snprintf(out, size, "%s/%s", base, name);
    if (n < 0 || (size_t)n >= size) {
        KART_LOG("path too long: '%s/%s'\n", base, name);
        return -1;
    }
    return 0;
}

static int file_new(const char *path)
{
    FILE *f = fopen(path, "w+");
    if (f == NULL) {
        KART_LOG("could not create file '%s': %s\n", path, strerror(errno));
        return -1;
    }
    fclose(f);
    return 0;
}

static int folder_new(const char *path)
{
    if (mkdir(path, 0755) != 0) {
        KART_LOG("could not create folder '%s': %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int cmd_new(const char *name)
{
    static const char *const dirs[]  = { "src", "include", "target", "tests" };
    static const char *const files[] = { "src/main.c", ".gitignore" };
    char path[PATH_MAX];

    KART_LOG("creating new project '%s'\n", name);

    if (folder_new(name) != 0)
        return 1;

    for (size_t i = 0; i < sizeof dirs / sizeof dirs[0]; i++) {
        if (path_join(path, sizeof path, name, dirs[i]) != 0 ||
            folder_new(path) != 0)
            return 1;
    }

    for (size_t i = 0; i < sizeof files / sizeof files[0]; i++) {
        if (path_join(path, sizeof path, name, files[i]) != 0 ||
            file_new(path) != 0)
            return 1;
    }

    KART_LOG("project created successfully\n");
    return 0;
}

// TODO: implement commands
static int cmd_run(void)   { KART_LOG("run: not implemented\n");   return 1; }
static int cmd_build(void) { KART_LOG("build: not implemented\n"); return 1; }
static int cmd_test(void)  { KART_LOG("test: not implemented\n");  return 1; }
static int cmd_clean(void) { KART_LOG("clean: not implemented\n"); return 1; }

#define OPTIONAL_ARGS \
    OPTIONAL_STRING_ARG(operation_new, NULL, "-n", "name", "Create a new project with a given name")

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(operation_run, "-r", "Run project") \
    BOOLEAN_ARG(operation_build, "-b", "Build project") \
    BOOLEAN_ARG(operation_test, "-t", "Run testing modules") \
    BOOLEAN_ARG(operation_clean, "-c", "Clear project artifacts") \
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

    if (args.operation_new)
        return cmd_new(args.operation_new);
    if (args.operation_run)
        return cmd_run();
    if (args.operation_build)
        return cmd_build();
    if (args.operation_test)
        return cmd_test();
    if (args.operation_clean)
        return cmd_clean();

    return 0;
}
