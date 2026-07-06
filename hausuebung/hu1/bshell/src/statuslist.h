#ifndef STATUSLIST_H
#define STATUSLIST_H

#include "list.h"
#include <sys/types.h>

typedef struct {
    pid_t pid;
    pid_t pgid;
    int raw_status;
    int running;
    char *prog;
} ProcEntry;

extern List *statuslist;

/* Fügt einen laufenden Hintergrundprozess hinzu */
void statuslist_add(pid_t pid, pid_t pgid, const char *prog);

/* Fügt einen Prozess hinzu, der schon beendet ist */
void statuslist_add_terminated(pid_t pid, pid_t pgid, const char *prog, int raw_status);

/* Aktualisiert den Zustand aller Prozesse */
void statuslist_update_all(void);

/* Gibt alle Prozesse aus und löscht beendete */
void statuslist_print_and_cleanup(void);

#endif /* STATUSLIST_H */
