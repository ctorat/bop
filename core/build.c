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
#include <assert.h>
#include "bop/hash.h"
#include "bop/common.h"
#include "bop/build.h"

/*
 * Build parameters
 *
 * @cflags:         List of CFLAGS
 * @hashfd:         Manifest file descriptor
 * @n_file:         Number of files to build
 * @gen_manifest:   If true, generate a manifest
 */
struct build_params {
    char *cflags;
    int hashfd;
    size_t n_file;
    bool gen_manifest;
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

/*
 * Obtain the filename from a manifest line
 *
 * XXX: Returns memory allocate with strdup(), remember to free!
 */
static char *
manifest_filename(const char *manifest_line)
{
    char *p, *p1;

    if (manifest_line == NULL) {
        return NULL;
    }

    if ((p = strdup(manifest_line)) == NULL) {
        printf("fatal: out of memory\n");
        return NULL;
    }

    p1 = p;
    while (*p1 != '\0') {
        if (*p1++ == ':') {
            *--p1 = '\0';
            return p;
        }
    }

    free(p);
    return NULL;
}

/*
 * Returns true if the given file is accepted as a buildable entity
 *
 * @filename:  Filename to check
 */
static bool
file_accepted(const char *filename)
{
    const char *ext;

    if (filename == NULL) {
        return false;
    }

    ext = get_extension(filename);
    if (strcmp(ext, ".c") == 0)
        return true;

    return false;
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
compile(const char *dirpath, const char *filename, struct build_params *params)
{
    const char *ext;
    char pathbuf[256];
    char hash[BOP_HASH_LEN];
    char manifest_entry[128];
    int error;

    if (dirpath == NULL || filename == NULL) {
        return -1;
    }

    if (params == NULL) {
        return -1;
    }

    ext = get_extension(filename);

    /* Ignore files with no extensions */
    if (ext == NULL) {
        return 0;
    }

    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dirpath, filename);

    if ((error = bop_hash_file(pathbuf, hash)) < 0) {
        printf("fatal: failed to hash %s\n", pathbuf);
        return error;
    }

    /* Used per accepted file to write hashes */
#define WRITE_HASH()                                                                \
    snprintf(manifest_entry, sizeof(manifest_entry), "%s:%s\n", filename, hash);    \
    write(params->hashfd, manifest_entry, strlen(manifest_entry));

    /* Build all CFILES */
    if (strcmp(ext, ".c") == 0) {
        error = invoke_cc(pathbuf, params);
        if (error < 0) {
            printf("fatal: cc invocation failure\n");
            return error;
        }

        if (params->gen_manifest) {
            WRITE_HASH();
        }
        return error;
    }

#undef WRITE_HASH
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

static int
build_from_dir(const char *dirpath, DIR *dir, build_op_t bop, struct build_params *params)
{
    struct dirent *dirent;
    int error;

    if (dir == NULL) {
        return -1;
    }

    /* Iterate each file */
    while ((dirent = readdir(dir)) != NULL) {
        if (dirent->d_name[0] == '.') {
            continue;
        }

        switch (bop) {
        case BUILD_OP_BUILD:
            error = compile(dirpath, dirent->d_name, params);
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

static int
try_mf_make(const char *dirpath, const char *manifest_entry,
    struct build_params *params, char hashres[BOP_HASH_LEN],
    bool *rehash)
{
    char *p, *tok, *name;
    char *save;
    char pathbuf[256];
    char hash[BOP_HASH_LEN];
    int error;

    if (dirpath == NULL || manifest_entry == NULL) {
        return -1;
    }

    if (rehash == NULL) {
        return -1;
    }

    *rehash = false;
    if ((p = strdup(manifest_entry)) == NULL) {
        printf("fatal: out of memory\n");
        return -1;
    }

    /* Grab the filename and construct a path */
    tok = strtok_r(p, ":", &save);
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dirpath, tok);

    /* Copy the name */
    if ((name = strdup(tok)) == NULL) {
        printf("fatal: out of memory\n");
        return -1;
    }

    /* Grab the old hash and create a new hash */
    tok = strtok_r(NULL, ":", &save);
    if ((error = bop_hash_file(pathbuf, hash)) < 0) {
        printf("[!] failed to hash %s\n", pathbuf);
        free(p);
        free(name);
        return -1;
    }

    tok[strlen(tok) - 1] = '\0';
    if (strncmp(tok, hash, sizeof(hash)) != 0) {
        error = compile(dirpath, name, params);
        if (error < 0) {
            printf("fatal: failed to build '%s'\n", name);
            free(p);
            free(name);
            return -1;
        }

        *rehash = true;
    }

    memcpy(hashres, hash, sizeof(hash));
    free(p);
    free(name);
    return 0;
}

static int
build_from_manifest(const char *dirpath, int hashfd, struct build_params *params)
{
    char line[128], *filename;
    char updated_line[128];
    char newhash[BOP_HASH_LEN];
    FILE *fp;
    size_t exp_n_file, pos;
    size_t oldpos;
    bool rehash;

    if (dirpath == NULL || params == NULL) {
        return -1;
    }

    if ((fp = fdopen(hashfd, "r+")) == NULL) {
        perror("fdopen");
        return -1;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        printf("fatal: unable to read manifest\n");
        fclose(fp);
        return -1;
    }

    /*
     * The first line is the expected file count, if they do not match then we are to
     * rebuild the entire manifest.
     */
    exp_n_file = atoi(line);
    if (params->n_file != exp_n_file) {
        return -1;
    }

    params->gen_manifest = false;
    pos = ftell(fp);

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (try_mf_make(dirpath, line, params, newhash, &rehash) < 0) {
            fclose(fp);
            return -1;
        }

        if (rehash) {
            oldpos = ftell(fp);
            if ((filename = manifest_filename(line)) == NULL)
                return -1;

            /* Update the line */
            fseek(fp, pos, SEEK_SET);
            snprintf(updated_line, sizeof(updated_line), "%s:%s", filename, newhash);
            fwrite(updated_line, 1, strlen(updated_line), fp);

            /* Reset position and free the filename */
            fseek(fp, oldpos, SEEK_SET);
            free(filename);
        }

        pos = ftell(fp);
    }

    fclose(fp);
    return 0;
}

int
bop_build_dir(const char *dirpath, build_op_t bop)
{
    struct build_params params;
    DIR *dir;
    struct dirent *dirent;
    char pathbuf[128];
    char countbuf[16];
    int error, hashfd = -1;
    bool build_dirs = true;

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

    /* Count the number of directory entries */
    while ((dirent = readdir(dir)) != NULL) {
        if (dirent->d_name[0] == '.') {
            continue;
        }

        if (file_accepted(dirent->d_name)) {
            ++params.n_file;
        }
    }

    rewinddir(dir);
    params.gen_manifest = true;

    /*
     * If a manifest is not present then we should create one, otherwise
     * we are to read it build based on it.
     */
    snprintf(pathbuf, sizeof(pathbuf), "%s/.bop/manifest.hash", dirpath);
    if (access(pathbuf, F_OK) != 0) {
        hashfd = open(pathbuf, O_RDWR | O_CREAT, 0666);
        if (hashfd < 0) {
            perror("open");
            printf("fatal: failed to open manifest\n");
            return -1;
        }

        params.hashfd = hashfd;
        snprintf(countbuf, sizeof(countbuf), "%zu\n", params.n_file);
        write(hashfd, countbuf, strlen(countbuf));
    } else {
        hashfd = open(pathbuf, O_RDWR);
        if (hashfd < 0) {
            perror("open");
            printf("fatal: failed to open manifest\n");
            return -1;
        }

        build_dirs = false;
        params.hashfd = hashfd;
        error = build_from_manifest(dirpath, hashfd, &params);

        /* Remove manifest and build from dir if error */
        if (error < 0) {
            printf("[*] flushing manifest\n");
            remove(pathbuf);
            build_dirs = true;
        }
    }

    /* Remove the manifest if we are cleaning */
    if (bop == BUILD_OP_CLEAN) {
        build_dirs = true;
        remove(pathbuf);
    }

    /* Build from directory if not manifest */
    if (build_dirs) {
        error = build_from_dir(dirpath, dir, bop, &params);
        if (error < 0)
            return error;
    }

    if (params.cflags != NULL) {
        free(params.cflags);
        params.cflags = NULL;
    }

    close(hashfd);
    return 0;
}
