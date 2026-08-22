/*
 * Copyright (c) 2026, Apollo Telephone Laboratories.
 * Provided under the BSD-3 clause.
 */

#ifndef BOP_BUILD_H
#define BOP_BUILD_H 1

/*
 * Build operations that can be performed
 *
 * @BUILD_OP_BUILD:  Build all CFILES and ASMFILES
 * @BUILD_OP_CLEAN:  Clean all object files
 */
typedef enum {
    BUILD_OP_BUILD,
    BUILD_OP_CLEAN
} build_op_t;

/*
 * Build a directory
 *
 * @dirpath:  Directory to build
 * @bop:      Build operation to perform
 *
 * Returns zeron on success
 */
int bop_build_dir(const char *dirpath, build_op_t bop);

#endif  /* !BOP_BUILD_H */
