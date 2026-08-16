/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "bop/common.h"
#include "bop/hook.h"
#include "bop/build.h"

static void
version(void)
{
    printf("Version: %s\n", BOP_VERSION);
}

static void
help(void)
{
    printf("usage: %s=<dir> ./bop [flags] [build_op]\n", BOP_BUILD_ENV);
    printf("-- flags --\n");
    printf("[-h]    Display this help menu\n");
    printf("[-v]    Display the version\n");
    printf("-- build operations\n");
    printf("[build]     Build the directory specified by %s\n", BOP_BUILD_ENV);
}

/*
 * Discover any possible hooks to be ran in the desired build directory and
 * begin the build process.
 */
static int
run_build(void)
{
    char pathbuf[256];
    char *build_dir;
    int error;

    build_dir = getenv(BOP_BUILD_ENV);
    if (build_dir == NULL) {
        printf("fatal: build directory not specified in %s\n", BOP_BUILD_ENV);
        return -1;
    }
    
    /*
     * There is a pre-build hook to be located within the [BUILD_DIR]/.bop/prehook.sh which
     * is simply a shell script to be executed.
     */ 
    snprintf(pathbuf, sizeof(pathbuf), "%s/.bop/prehook.sh", build_dir);
    if (access(pathbuf, F_OK) == 0) {
        bop_shell_hook(pathbuf);
    }

    error = bop_build_dir(build_dir);
    if (error < 0) {
        printf("fatal: failed to build '%s'\n", build_dir);
        return error;
    }
    
    /* Run the post hook if we can */
    snprintf(pathbuf, sizeof(pathbuf), "%s/.bop/posthook.sh", build_dir);
    if (access(pathbuf, F_OK) == 0) {
        bop_shell_hook(pathbuf);
    }

    return 0;
}

static int
do_build_op(const char *op)
{
    if (op == NULL) {
        return -1;
    }

    switch (*op) {
    case 'b':
        if (strcmp(op, "build") == 0) {
            return run_build();
        }

        break;
    }
    
    return -1;
}

int
main(int argc, char **argv)
{
    int opt;
    char *build_opt;

    if (argc < 2) {
        printf("fatal: too few arguments\n");
        help();
        return -1;
    }

    while ((opt = getopt(argc, argv, "hv")) != -1) {
        switch (opt) {
        case 'h':
            help();
            return -1;
        case 'v':
            version();
            return -1;
        }
    }

    if (optind >= argc) {
        printf("fatal: please specify a build operation\n");
        help();
        return -1;
    }

    build_opt = argv[optind++];
    return do_build_op(build_opt);
}
