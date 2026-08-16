/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#include <sys/wait.h>
#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "bop/common.h"
#include "bop/build.h"

/*
 * Build parameters
 *
 * @cflags: List of CFLAGS
 *
 * TODO: Use this instead of BOP_DEFAULT_CFLAGS and similar
 */
struct build_params {
    char **cflags;
};

/*
 * Obtain the file extension of a file
 *
 * @filename: Filename to obtain extension of
 */
static const char *
get_extension(const char *filename)
{
    const char *p;
    bool have_dot = false;

    if (filename == NULL) {
        return NULL;
    }

    p = filename;
    while (*p != '\0') {
        if (*p == '.') {
            have_dot = true;
        } else if (have_dot) {
            --p;
            return p;
        }
        ++p;
    }

    return NULL;
}

/*
 * Truncate a file extension
 *
 * @filename: Filename to truncate file extension from
 *
 * XXX: Returns memory allocated with strdup(), remember to free!
 */
static char *
trunc_extension(const char *filename)
{
    char *p, *p1;

    if ((p = strdup(filename)) == NULL) {
        return NULL;
    }

    p1 = p;
    while (*p1 != '\0') {
        if (*p1 == '.') {
            *p1 = '\0';
            return p;
        }
        ++p1;
    }

    free(p);
    return NULL;
}

static int
invoke_cc(const char *cfile_path, struct build_params *params)
{
    pid_t child;
    int status;
    char *trunc_ext;
    char outpath[256];

    if (cfile_path == NULL || params == NULL) {
        return -1;
    }

    trunc_ext = trunc_extension(cfile_path);
    if (trunc_ext == NULL) {
        return -1;
    }

    snprintf(outpath, sizeof(outpath), "%s.o", trunc_ext);
    child = fork();
    if (child == 0) {
        printf("[CC]\t\t%s\n", cfile_path);
        execl(BOP_CC, BOP_CC, "-c", cfile_path, "-o", outpath, BOP_DEFAULT_CFLAGS, NULL);
    } else {
        waitpid(child, &status, 0);
        free(trunc_ext);
        return status;
    }

    return -1;
}

static int
compile(const char *dirpath, struct dirent *dirent, struct build_params *params)
{
    const char *filename, *ext;
    char pathbuf[256];
    int error;

    if (dirpath == NULL || dirent == NULL) {
        return -1;
    }

    /* Ignore non regulars */
    if (dirent->d_type != DT_REG) {
        return 0;
    }

    filename = dirent->d_name;
    ext = get_extension(filename);

    /* Ignore files with no extensions */
    if (ext == NULL) {
        return 0;
    }

    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dirpath, filename);

    /* Build all CFILES */
    if (strcmp(ext, ".c") == 0) {
        error = invoke_cc(pathbuf, params);
        if (error < 0)
            printf("fatal: cc invocation failure\n");
        return error;
    }

    return 0;

}

static void
clean_object(const char *dirpath, struct dirent *dirent)
{
    char pathbuf[256];
    const char *filename, *ext;

    if (dirpath == NULL || dirent == NULL) {
        return;
    }

    /* Ignore non regulars */
    if (dirent->d_type != DT_REG) {
        return;
    }

    /* Ignore files without an extension */
    filename = dirent->d_name;
    ext = get_extension(filename);
    if (ext == NULL) {
        return;
    }

    /* Ignore non object files */
    if (strcmp(ext, ".o") != 0) {
        return;
    }

    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dirpath, dirent->d_name);
    printf("clean %s\n", pathbuf);
    remove(pathbuf);
}

int
bop_build_dir(const char *dirpath, build_op_t bop)
{
    struct build_params params;
    struct dirent *dirent;
    DIR *dir;
    int error;

    dir = opendir(dirpath);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    bzero(&params, sizeof(params));
    while ((dirent = readdir(dir)) != NULL) {
        if (dirent->d_name[0] == '.') {
            continue;
        }

        switch (bop) {
        case BUILD_OP_BUILD:
            error = compile(dirpath, dirent, &params);
            if (error < 0) {
                printf("fatal: failed to build '%s'\n", dirent->d_name);
                return error;
            }

            break;
        case BUILD_OP_CLEAN:
            clean_object(dirpath, dirent);
            break;
        }

    }

    return 0;
}
