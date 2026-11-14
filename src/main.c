#define _XOPEN_SOURCE 700
#include <linux/limits.h>
#include <stdint.h>
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
#include "tomlc17/tomlc17.h"

config_t config = {0};
static const char *const dirs[]  = { "src", "include", "target", "tests" };
static const char *const files[] = { "src/main.c", ".gitignore", KART_CONFIG_FILENAME };

static const field_t fields[F_COUNT] = {
    [F_NAME]    = {"package.name"   , F_NAME   },
    [F_VERSION] = {"package.version", F_VERSION},
    [F_STD]     = {"package.std"    , F_STD    },
    [F_CFLAGS]  = {"build.cflags"   , F_CFLAGS },
    [F_LDFLAGS] = {"build.ldflags"  , F_LDFLAGS},
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
    strncat(command, "./target/", 10);
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

static int cmd_build(void)
{
    LOG_INF("building '%s' project\n", config.name);

    char command[ARG_MAX] = {0};

    // Create base build command
    STRNCAT_LITERAL_STR(command, "gcc ");

    // Append cflags
    if (config.cflags.u.arr.size > 0) {
        for (int i = 0; i < config.cflags.u.arr.size; ++i) {
            const char* curr_elem = config.cflags.u.arr.elem[i].u.s;
            strncat(command, curr_elem, strlen(curr_elem));
            STRNCAT_EMPTY_SPACE();
        }
    }

    // Append compiler standard
    if (config.std != NULL && config.std[0] != '\0') {
        STRNCAT_LITERAL_STR(command, "-std=");
        strncat(command, config.std, strlen(config.std));
        STRNCAT_EMPTY_SPACE();
    }

    // Append all files at src/
    file_append_into("src", (char*)&command);

    // Appends ldflags
    if (config.ldflags.u.arr.size > 0) {
        for (int i = 0; i < config.ldflags.u.arr.size; ++i) {
            const char* curr_elem = config.ldflags.u.arr.elem[i].u.s;
            strncat(command, curr_elem, strlen(curr_elem));
            STRNCAT_EMPTY_SPACE();
        }
    }

    // TODO: Append ldlibs

    // Output path
    STRNCAT_LITERAL_STR(command, "-o target/");
    strncat(command, config.name, strlen(config.name));

    LOG_INF("running: '%s'\n", command);
    system(command);

    return 0;
}

// TODO: implement commands
static int cmd_install(void) { LOG_ERR("install: not implemented\n"); return 1; }
static int cmd_test(void)    { LOG_ERR("test: not implemented\n");    return 1; }

#define OPTIONAL_ARGS \
    OPTIONAL_STRING_ARG(operation_new, "", "-n", "name", "Create a new project with a given name")

#define BOOLEAN_ARGS \
    BOOLEAN_ARG(operation_run, "-r", "Run project") \
    BOOLEAN_ARG(operation_build, "-b", "Build project") \
    BOOLEAN_ARG(operation_install, "-i", "Instal project system-wide") \
    BOOLEAN_ARG(operation_test, "-t", "Run testing modules") \
    BOOLEAN_ARG(operation_clean, "-c", "Clear project artifacts") \
    BOOLEAN_ARG(enable_hot_reloading, "--hot-reloading", "Enables hot-reloading") \
    BOOLEAN_ARG(help, "-h", "Show help")

#include "easyargs.h"

static inline int field_check_type(toml_datum_t field, field_id id, toml_type_t type)
{
    if (field.type == TOML_UNKNOWN) {
        LOG_WRN("'%s' is not set\n", fields[id].key);
        return 0;
    } else if (field.type != type) {
        LOG_ERR("missing or invalid '%s' property at '%s'\n", fields[id].key, KART_CONFIG_FILENAME);
        toml_free(config.result);
        memset(&config, 0, sizeof config);
        return 1;
    }
    return 0;
}

// TODO: Alert for all the errors found at a given field instead of alerting just the first one found
static inline int elem_check_type(toml_datum_t field, int field_id, int elements_count, toml_type_t type)
{
    for (int i = 0; i < elements_count; ++i) {
        toml_datum_t elem = field.u.arr.elem[i];

        if (elem.type != type) {
            LOG_ERR("Invalid element type at '%s'\n", fields[field_id].key);
            toml_free(config.result);
            memset(&config, 0, sizeof config);
            return 1;
        }
    }
    return 0;
}

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

    // Get fields info
    for (int i = 0; i < F_COUNT; i++) {
        toml_datum_t curr_field = toml_seek(config.result.toptab, fields[i].key);

        switch (i) {
            case F_NAME:
                if (field_check_type(curr_field, i, TOML_STRING)) return 1;

                config.name = (char*)curr_field.u.s;
                break;
            case F_VERSION:
                if (field_check_type(curr_field, i, TOML_STRING)) return 1;

                config.version = (char*)curr_field.u.s;
                break;
            case F_STD:
                if (field_check_type(curr_field, i, TOML_STRING)) return 1;

                config.std = (char*)curr_field.u.s;
                break;
            // TODO: Think about improving array checking abstraction, all the arrays at the moment 
            // can use the same code, but it doesn't mean all the possible arrays will do the same
            case F_CFLAGS:
                if (field_check_type(curr_field, i, TOML_ARRAY)) return 1;
                if (elem_check_type(curr_field, i, curr_field.u.arr.size, TOML_STRING)) return 1;
                
                config.cflags = curr_field;
                break;
            case F_LDFLAGS:
                if (field_check_type(curr_field, i, TOML_ARRAY)) return 1;
                if (elem_check_type(curr_field, i, curr_field.u.arr.size, TOML_STRING)) return 1;

                config.ldflags = curr_field;
                break;
        }
    }

    if (args.operation_run) {
        // Building complete path 
        char exec_path[4096] = {0};
        path_join(exec_path, sizeof exec_path, "target", config.name);

        LOG_INF("looking up for '%s'\n", exec_path);
        r = file_exists(exec_path);
        if (r != 0) {
            LOG_ERR("could not find binary '%s'\n", exec_path);
            return 1;
        }

        // Warning: when running the program, all other arguments passed into
        // kart are going to be taken as arguments for the program itself
        char** args = (argc >= 3) ? &argv[2] : NULL;
        return cmd_run(args, argc - 2);
    } else if (args.operation_build) {
        return cmd_build();
    } else if (args.operation_install) {
        return cmd_install();
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
