/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#ifndef BOP_HASH_H
#define BOP_HASH_H 1

#include <stdint.h>
#include <stddef.h>
#include <openssl/sha.h>

#define BOP_HASH_LEN 65

/*
 * Hash a file with SHA256
 *
 * @path:       Path of file to hash
 * @hashres:    Hash result is written here
 *
 * Returns zero on success
 */
int bop_hash_file(const char *path, char hashres[BOP_HASH_LEN]);

#endif  /* !BOP_HASH_H */
