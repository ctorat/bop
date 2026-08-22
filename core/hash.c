/*
 * Copyright (c) 2026, Apollo Telephone Laboratories.
 * Provided under the BSD-3 clause.
 */

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "bop/hash.h"

int
bop_hash_file(const char *path, char hashres[BOP_HASH_LEN])
{
    int fd;
    size_t len, i;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    void *mem;

    if (path == NULL) {
        return -1;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        perror("open");
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

    SHA256(mem, len, hash);
    for (i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf((char *)hashres + i*2, "%02X", hash[i]);
    }

    munmap(mem, len);
    close(fd);
    return 0;
}
