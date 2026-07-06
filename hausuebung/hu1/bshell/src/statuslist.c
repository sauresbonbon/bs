#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "statuslist.h"
#include "list.h"

List *statuslist = NULL;

/* Block SIGCHLD and save old mask — call from main loop only */
static void block_sigchld(sigset_t *old) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, old);
}

static void unblock_sigchld(const sigset_t *old) {
    sigprocmask(SIG_SETMASK, old, NULL);
}

/* Append to end of list — caller must hold SIGCHLD block */
static void append_unsafe(ProcEntry *entry) {
    List *node = list_append(entry, NULL);
    if (statuslist == NULL) {
        statuslist = node;
    } else {
        List *lst = statuslist;
        while (lst->tail != NULL)
            lst = lst->tail;
        lst->tail = node;
    }
}

void statuslist_add(pid_t pid, pid_t pgid, const char *prog) {
    ProcEntry *entry = malloc(sizeof(ProcEntry));
    entry->pid = pid;
    entry->pgid = pgid;
    entry->raw_status = 0;
    entry->running = 1;
    entry->prog = strdup(prog);

    sigset_t old;
    block_sigchld(&old);
    append_unsafe(entry);
    unblock_sigchld(&old);
}

void statuslist_add_terminated(pid_t pid, pid_t pgid, const char *prog, int raw_status) {
    ProcEntry *entry = malloc(sizeof(ProcEntry));
    entry->pid = pid;
    entry->pgid = pgid;
    entry->raw_status = raw_status;
    entry->running = 0;
    entry->prog = strdup(prog);

    sigset_t old;
    block_sigchld(&old);
    append_unsafe(entry);
    unblock_sigchld(&old);
}

/*
 * Non-blocking waitpid for every running entry.
 * Called from the SIGCHLD handler — must NOT block signals or call
 * malloc/free (async-signal-safety requirement).
 */
void statuslist_update_all(void) {
    List *lst = statuslist;
    while (lst != NULL) {
        ProcEntry *entry = (ProcEntry *)lst->head;
        if (entry->running) {
            int status;
            pid_t result = waitpid(entry->pid, &status, WNOHANG);
            if (result > 0) {
                entry->running = 0;
                entry->raw_status = status;
            }
        }
        lst = lst->tail;
    }
}

void statuslist_print_and_cleanup(void) {
    sigset_t old;
    block_sigchld(&old);

    /* Catch any zombies the handler may have missed */
    statuslist_update_all();

    printf("%-8s %-8s %-12s %s\n", "PID", "PGID", "STATUS", "PROG");

    List *lst = statuslist;
    while (lst != NULL) {
        ProcEntry *entry = (ProcEntry *)lst->head;
        char status_str[32];
        if (entry->running) {
            snprintf(status_str, sizeof(status_str), "running");
        } else if (WIFEXITED(entry->raw_status)) {
            snprintf(status_str, sizeof(status_str), "exit(%d)", WEXITSTATUS(entry->raw_status));
        } else if (WIFSIGNALED(entry->raw_status)) {
            snprintf(status_str, sizeof(status_str), "signal(%d)", WTERMSIG(entry->raw_status));
        } else {
            snprintf(status_str, sizeof(status_str), "unknown");
        }
        printf("%-8d %-8d %-12s %s\n", entry->pid, entry->pgid, status_str, entry->prog);
        lst = lst->tail;
    }

    /* Remove terminated entries in-place (pointer-to-pointer traversal) */
    List **ptr = &statuslist;
    while (*ptr != NULL) {
        List *current = *ptr;
        ProcEntry *entry = (ProcEntry *)current->head;
        if (!entry->running) {
            *ptr = current->tail;
            free(entry->prog);
            free(entry);
            free(current);
        } else {
            ptr = &current->tail;
        }
    }

    unblock_sigchld(&old);
}
