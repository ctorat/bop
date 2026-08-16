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
    printf("[clean]     Clean all object files\n");
}

/*
 * Convert an operations string to a build_op_t type
 */
static inline build_op_t
opstr_to_op(const char *opstr)
{
    if (opstr == NULL) {
        return BUILD_OP_BUILD;
    }

    switch (*opstr) {
    case 'b':
        if (strcmp(opstr, "build") == 0) {
            return BUILD_OP_BUILD;
        }

        break;
    case 'c':
        if (strcmp(opstr, "clean") == 0) {
            return BUILD_OP_CLEAN;
        }

        break;
    }

    return BUILD_OP_BUILD;
}

/*
 * Discover any possible hooks to be ran in the desired build directory and
 * begin the build process.
 *
 * @build_dir:  Build directory
 * @bop:        Build operation
 */
static int
run_build(const char *build_dir, build_op_t bop)
{
    char pathbuf[256];
    int error;

    /*
     * There is a pre-build hook to be located within the [BUILD_DIR]/.bop/prehook.sh which
     * is simply a shell script to be executed. Only run it if we aren't cleaning up the build
     * space.
     */
    if (bop != BUILD_OP_CLEAN) {
        snprintf(pathbuf, sizeof(pathbuf), "%s/.bop/prehook.sh", build_dir);
        if (access(pathbuf, F_OK) == 0)
            bop_shell_hook(pathbuf);
    }

    error = bop_build_dir(build_dir, bop);
    if (error < 0) {
        printf("fatal: failed to build '%s'\n", build_dir);
        return error;
    }

    /* Run the post hook if we can */
    if (bop != BUILD_OP_CLEAN) {
        snprintf(pathbuf, sizeof(pathbuf), "%s/.bop/posthook.sh", build_dir);
        if (access(pathbuf, F_OK) == 0)
            bop_shell_hook(pathbuf);
    }

    return 0;
}

/*
 * Build a directory list
 *
 * @bop:      Build operations
 */
static int
build_dirlist(build_op_t bop)
{
    const char *dir_list;
    char *p, *tok;
    int error;

    dir_list = getenv(BOP_BUILD_ENV);
    if (dir_list == NULL) {
        printf("fatal: build directories not specified in %s\n", BOP_BUILD_ENV);
        return -1;
    }

    if ((p = strdup(dir_list)) == NULL) {
        printf("fatal: out of memory\n");
        return -1;
    }

    /* Build each directory specified */
    tok = strtok(p, ":");
    while (tok != NULL) {
        printf("[->]\t\t%s\n", tok);
        error = run_build(tok, bop);
        if (error < 0)
            break;
        tok = strtok(NULL, ":");
    }

    if (error < 0) {
        printf("fatal: failed to build %s\n", tok);
        free(p);
        return -1;
    }

    free(p);
    return 0;
}

int
main(int argc, char **argv)
{
    int opt;
    char *build_opt;
    build_op_t bop;

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
    bop = opstr_to_op(build_opt);
    return build_dirlist(bop);
}
