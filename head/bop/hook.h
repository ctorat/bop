/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#ifndef BOP_HOOK_H
#define BOP_HOOK_H 1

/*
 * Run a shell script build hook
 *
 * @hook_path:  Path to hook
 *
 * Returns zero on success
 */
int bop_shell_hook(const char *hook_path);

#endif  /* !BOP_HOOK_H */
