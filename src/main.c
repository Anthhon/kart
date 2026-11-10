#include <linux/limits.h>
#define _XOPEN_SOURCE 700
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "main.h"
#include "folder.h"
#include "file.h"
#include "helpers.h"
#include "../include/tomlc17/tomlc17.h"

config_t config = {0};
static const char *const dirs[]  = { "src", "include", "target", "tests" };
static const char *const files[] = { "src/main.c", ".gitignore", KART_CONFIG_FILENAME };
static const field_t fields[F_COUNT] = {
    [F_NAME]    = {"package.name",    F_NAME},
    [F_VERSION] = {"package.version", F_VERSION},
    [F_STD]     = {"package.std",     F_STD},
};

static int cmd_new(const char *name)
{
    for (int i = 0; i < ELEMENTS_COUNT(dirs); i++) {
        if (strcmp(name, dirs[i]) == 0) {
            LOG_ERR("Invalid name for project folder\n");
            return 1;
        }
    }

    char path[PATH_MAX] = {0};

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

static int cmd_run(char* args[], int argc)
{
    char command[ARG_MAX] = {0};

    // Create base run command
    strncat(command, "./", 3);
    strncat(command, config.name, strlen(config.name));

    // Append arguments to run command
    if (args != NULL) {
        for (int i = 0; i < argc; i++) {
            strncat(command, " ", 2);
            strncat(command, args[i], strlen(args[i]));
        } }

    LOG_INF("running: %s\n", command);
    system(command);

    return 0;
}

// TODO: implement commands
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
    int dash = -1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-r") == 0) {
            dash = i + 1;
            break;
        }
    }

    int r = 0;
    args_t args = make_default_args();

    if (argc <= 1 || !parse_args(((dash == -1) ? argc : 2), argv, &args)) {
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
    }

    // Safeguard with kart config file
    r = file_exists(KART_CONFIG_FILENAME);
    if (r != 0) {
        LOG_WRN("could not find '%s' file\n", KART_CONFIG_FILENAME);
        return 1;
    }

    config.result = toml_parse_file_ex(KART_CONFIG_FILENAME);
    if (!config.result.ok) {
        LOG_ERR("tomlc17: %s\n", config.result.errmsg);
        memset(&config, 0, sizeof config);
        return 1;
    }

    // Get required fields
    for (int i = 0; i < F_COUNT; i++) {
        toml_datum_t field = toml_seek(config.result.toptab, fields[i].key);
        if (field.type != TOML_STRING) {
            LOG_ERR("missing or invalid '%s' property in config\n", fields[i].key);
            toml_free(config.result);
            memset(&config, 0, sizeof config);
            return 1;
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

    if (args.operation_run) {
        LOG_INF("Looking up for '%s'\n", config.name);
        r = file_exists(config.name);
        if (r != 0) {
            LOG_ERR("could not find binary '%s'\n", config.name);
            return 1;
        }

        // Warning: when running the program, all other arguments passed into
        // kart are going to be taken as arguments for the program itself
        char** args = (argc >= 3) ? &argv[2] : NULL;
        return cmd_run(args, argc - 2);
    } else if (args.operation_build) {
        return cmd_build();
    } else if (args.operation_test) {
        return cmd_test();
    } else if (args.operation_clean) {
        return cmd_clean();
    }

    // Clean-up config content
    toml_free(config.result);
    memset(&config, 0, sizeof(config));

    return 0;
}
