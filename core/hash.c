/*
 * Copyright (c) 2026, Apollo Telephone Laboratories.
 * Provided under the BSD-3 clause.
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "bop/hash.h"

int
bop_hash_file(const char *path, char hashres[BOP_HASH_LEN])
{
    int fd;
    size_t len, i;
    uint8_t *buf, hash[SHA256_DIGEST_LENGTH];
    void *mem;
    struct stat sb;

    if (path == NULL) {
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        perror("open");
        return -1;
    }

    if (stat(path, &sb) < 0) {
        perror("stat");
        close(fd);
        return -1;
    }

    /* Grab the file length */
    len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    /* Map the file */
    mem = mmap(
        NULL,
        len,
        PROT_READ,
        MAP_SHARED,
        fd,
        0
    );

    if (mem == NULL) {
        perror("mmap");
        close(fd);
        return -1;
    }

    /* Allocate the buffer for packing */
    buf = malloc(len + sizeof(sb.st_mtime));
    if (buf == NULL) {
        printf("fatal: failed to allocate hash buffer\n");
        munmap(mem, len);
        close(fd);
        return -1;
    }

    /* Pack together access time and contents */
    memcpy(buf, mem, len);
    memcpy(&buf[len], &sb.st_mtime, sizeof(sb.st_atime));
    SHA256(buf, len + sizeof(sb.st_atime), hash);

    for (i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf((char *)hashres + i*2, "%02X", hash[i]);
    }

    free(buf);
    munmap(mem, len);
    close(fd);
    return 0;
}
