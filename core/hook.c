/*
 * Copyright (c) 2026, Chloe M., et al.
 * Provided under the BSD-3 clause.
 */

#include <sys/wait.h>
#include <unistd.h>
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
    if (script_path == NULL) {
        return;
    }

    execl(DEFAULT_SHELL_PATH, DEFAULT_SHELL_PATH, script_path, NULL);
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
