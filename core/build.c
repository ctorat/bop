/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
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
 */
struct build_params {
    char *cflags;
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
    char *trunc_ext, *cflags;
    char cmdbuf[256];
    char outpath[256];
    char outbuf[32];
    FILE *pipe;

    if (cfile_path == NULL || params == NULL) {
        return -1;
    }

    trunc_ext = trunc_extension(cfile_path);
    if (trunc_ext == NULL) {
        return -1;
    }

    cflags = (params->cflags == NULL) ? "" : params->cflags;
    snprintf(outpath, sizeof(outpath), "%s.o", trunc_ext);
    snprintf(cmdbuf, sizeof(cmdbuf), "%s -c %s %s -o %s", BOP_CC, cflags, cfile_path, outpath);

    if ((pipe = popen(cmdbuf, "r")) == NULL) {
        printf("fatal: failed to invoke cc\n");
        return -1;
    }

    printf("[CC]\t\t%s\n", cfile_path);
    while ((fgets(outbuf, sizeof(outbuf), pipe)) != NULL) {
        printf("%s", outbuf);
    }

    pclose(pipe);
    return 0;
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

/*
 * Initialize the cflags field within build parameters
 *
 * @dirpath:  Target directory path
 * @params:   Build parameters
 *
 * Returns zero on success
 */
static int
build_init_cflags(const char *dirpath, struct build_params *params)
{
    char pathbuf[256], *databuf;
    size_t len, fsize;
    int fd;

    if (params == NULL) {
        return -1;
    }

    snprintf(pathbuf, sizeof(pathbuf), "%s/.bop/cflags", dirpath);
    if ((fd = open(pathbuf, O_RDONLY)) < 0) {
        /* Fake success, cflags are optional */
        return 0;
    }

    /* Grab the file size */
    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    /* Allocate the data buffer */
    if ((databuf = malloc(fsize)) == NULL) {
        printf("fatal: out of memory\n");
        close(fd);
        return -1;
    }

    if ((len = read(fd, databuf, fsize)) < 0) {
        perror("read");
        printf("fatal: failed to read %s\n", pathbuf);
        close(fd);
        return -1;
    }

    databuf[fsize - 1] = '\0';
    params->cflags = databuf;
    close(fd);
    return 0;
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
    if ((error = build_init_cflags(dirpath, &params)) < 0) {
        closedir(dir);
        return error;
    }

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

    if (params.cflags != NULL) {
        free(params.cflags);
        params.cflags = NULL;
    }

    return 0;
}
