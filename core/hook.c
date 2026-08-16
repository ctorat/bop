/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "bop/hook.h"

/* Shell we use by default */
#define DEFAULT_SHELL_PATH "/bin/sh"

/*
 * Execute the shell for a specific shell script
 *
 * @script_path:  Path of shell script
 */
static void
exec_shell_for(const char *script_path)
{
    char *shell_path;

    if (script_path == NULL) {
        return;
    }

    /*
     * We will attempt to use the current shell but if we cannot determine it,
     * we will then fallback to the default shell path.
     */
    shell_path = getenv("SHELL");
    if (shell_path == NULL) {
        shell_path = DEFAULT_SHELL_PATH;
        printf("warning: could not determine shell\n");
        printf("warning: falling back to '%s'\n", shell_path);
    }

    execl(shell_path, shell_path, script_path, NULL);
}

/*
 * Run a shell hook with POSIX sh
 *
 * XXX: Maybe support '#!/bin/xxx'?
 */
int
bop_shell_hook(const char *hook_path)
{
    pid_t child;
    int status;

    if (hook_path == NULL) {
        return -1;
    }

    child = fork();
    if (child == 0) {
        exec_shell_for(hook_path);
    } else {
        waitpid(child, &status, 0);
    }

    return 0;
}
