#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include "../include/tomlc17/tomlc17.h"

// Kart macros
#define KART_CONFIG_FILENAME "kart.toml"
#define KART_CONFIG_TEMPLATE              \
    "[package]\n"                         \
    "name = \"%s\"\n"                     \
    "version = \"0.1.0\"\n"               \
    "std = \"c11\"\n"                     \
    "\n"                                  \
    "[build]\n"                           \
    "cflags = [\"-Wall\", \"-Werror\"]\n" \
    "ldflags = []\n"                      \

// Log macros
#define ANSI_RESET  "\x1b[0m"
#define ANSI_BOLD   "\x1b[1m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_CYAN   "\x1b[36m"
#define ANSI_RED    "\x1b[31m"
#define LOG_WRN(...)  fprintf(stderr, ANSI_BOLD ANSI_YELLOW "[wrn]" ANSI_RESET " kart: " __VA_ARGS__)
#define LOG_INF(...)  fprintf(stdout, ANSI_BOLD ANSI_CYAN   "[inf]" ANSI_RESET " kart: " __VA_ARGS__)
#define LOG_ERR(...)  fprintf(stderr, ANSI_BOLD ANSI_RED    "[err]" ANSI_RESET " kart: " __VA_ARGS__)

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

int main(int argc, char *argv[]);

#endif // MAIN_H_
