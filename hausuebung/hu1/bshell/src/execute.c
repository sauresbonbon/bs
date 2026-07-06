#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "shell.h"
#include "helper.h"
#include "command.h"
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "statuslist.h"
#include "debug.h"
#include "execute.h"

/* do not modify this */
#ifndef NOLIBREADLINE
#include <readline/history.h>
#endif /* NOLIBREADLINE */

extern int shell_pid;
extern int fdtty;

/* do not modify this */
#ifndef NOLIBREADLINE
static int builtin_hist(char ** command){

    register HIST_ENTRY **the_list;
    register int i;
    printf("--- History --- \n");

    the_list = history_list ();
    if (the_list)
        for (i = 0; the_list[i]; i++)
            printf ("%d: %s\n", i + history_base, the_list[i]->line);
    else {
        printf("history could not be found!\n");
    }

    printf("--------------- \n");
    return 0;
}
#endif /*NOLIBREADLINE*/
void unquote(char * s){
	if (s!=NULL){
		if (s[0] == '"' && s[strlen(s)-1] == '"'){
	        char * buffer = calloc(sizeof(char), strlen(s) + 1);
			strcpy(buffer, s);
			strncpy(s, buffer+1, strlen(buffer)-2);
                        s[strlen(s)-2]='\0';
			free(buffer);
		}
	}
}

void unquote_command_tokens(char ** tokens){
    int i=0;
    while(tokens[i] != NULL) {
        unquote(tokens[i]);
        i++;
    }
}

void unquote_redirect_filenames(List *redirections){
    List *lst = redirections;
    while (lst != NULL) {
        Redirection *redirection = (Redirection *)lst->head;
        if (redirection->r_type == R_FILE) {
            unquote(redirection->u.r_file);
        }
        lst = lst->tail;
    }
}

void unquote_command(Command *cmd){
    List *lst = NULL;
    switch (cmd->command_type) {
        case C_SIMPLE:
        case C_OR:
        case C_AND:
        case C_PIPE:
        case C_SEQUENCE:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                SimpleCommand *cmd_s = (SimpleCommand *)lst->head;
                unquote_command_tokens(cmd_s->command_tokens);
                unquote_redirect_filenames(cmd_s->redirections);
                lst = lst->tail;
            }
            break;
        case C_EMPTY:
        default:
            break;
    }
}

static int execute_fork(SimpleCommand *cmd_s, int background){
    char ** command = cmd_s->command_tokens;
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid==0){
        /* child */
        signal(SIGINT, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        // Neu <, <<, >
        if (cmd_s->redirections != NULL){
            List *r = cmd_s->redirections;
            while (r != NULL) {
                Redirection *redirection = (Redirection *)r->head;
                int fd;

                if (redirection->r_type == R_FILE) {
                    if (redirection->r_mode == M_READ) {
                        fd = open(redirection->u.r_file, O_RDONLY);
                        if (fd < 0) {
                            if (errno == EACCES) {
                                fprintf(stderr, "-bshell: %s: Permission denied\n", redirection->u.r_file);
                            } else if (errno == ENOENT) {
                                fprintf(stderr, "-bshell: %s: No such file or directory\n", redirection->u.r_file);
                            }
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                    }
                    else if (redirection->r_mode == M_WRITE) {
                        fd = open(redirection->u.r_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        if (fd < 0) {
                            if (errno == EACCES) {
                                fprintf(stderr, "-bshell: %s: Permission denied\n", redirection->u.r_file);
                            } else if (errno == ENOENT) {
                                fprintf(stderr, "-bshell: %s: No such file or directory\n", redirection->u.r_file);
                            }
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                    else if (redirection->r_mode == M_APPEND) {
                        fd = open(redirection->u.r_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                        if (fd < 0) {
                            if (errno == EACCES) {
                                fprintf(stderr, "-bshell: %s: Permission denied\n", redirection->u.r_file);
                            } else if (errno == ENOENT) {
                                fprintf(stderr, "-bshell: %s: No such file or directory\n", redirection->u.r_file);
                            }
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                }

                r = r->tail;
            }
        }

        if (execvp(command[0], command) == -1){
            fprintf(stderr, "-bshell: %s : command not found \n", command[0]);
            perror("");
        }
        /*exec only return on error*/
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("shell");

    } else {
        /*parent*/
        setpgid(pid, pid);
        if (background == 0) {
            /* wait only if no background process */
            tcsetpgrp(fdtty, pid);

            /**
             * the handling here is far more complicated than this!
             * vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
             */

            wpid = waitpid(pid, &status, 0);
            (void)wpid;

            //^^^^^^^^^^^^^^^^^^^^^^^^^^^^

            tcsetpgrp(fdtty, shell_pid);

            /* Add foreground process to list after it is already reaped */
            statuslist_add_terminated(pid, pid, command[0], status);

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 1;
            }
            return 0;
        } else {
            /* background */
            fprintf(stderr, "PID=%d PGID=%d\n", pid, pid);
            statuslist_add(pid, pid, command[0]);
            return 0;
        }
    }

    return 0;
}


static int do_execute_simple(SimpleCommand *cmd_s, int background){
    if (cmd_s==NULL){
        return 0;
    }

    // Neu "cd" Builtin
    else if (strcmp(cmd_s->command_tokens[0],"cd")==0) {
        char *path = cmd_s->command_tokens[1];

        if (path == NULL || strcmp(cmd_s->command_tokens[1], "~") == 0) {
            path = getenv("HOME");
        }

        if (chdir(path) == -1) {

            if (errno == ENOENT) {
                fprintf(stderr, "-bshell: cd: %s: No such file or directory\n", path);
            }
            else if (errno == EACCES) {
                fprintf(stderr, "cd: %s: Permission denied\n", path);
            }
            return 1;
        }

        return  0;
    }


     else if (strcmp(cmd_s->command_tokens[0], "status") == 0) {
        statuslist_print_and_cleanup();
        return 0;

    } else if (strcmp(cmd_s->command_tokens[0],"exit")==0){
         char *exitcode = cmd_s->command_tokens[1];

         if (exitcode != NULL) {
             exit(atoi(exitcode));
         }

        exit(0);

/* do not modify this */
#ifndef NOLIBREADLINE
    }
    else if (strcmp(cmd_s->command_tokens[0],"hist")==0){
        return builtin_hist(cmd_s->command_tokens);
#endif /* NOLIBREADLINE */
    } else {
        return execute_fork(cmd_s, background);
    }
    fprintf(stderr, "This should never happen!\n");
    exit(1);

}

/*
 * check if the command is to be executed in back- or foreground.
 *
 * For sequences, the '&' sign of the last command in the
 * sequence is checked.
 *
 * returns:
 *      0 in case of foreground execution
 *      1 in case of background execution
 *
 */
int check_background_execution(Command * cmd){
    List * lst = NULL;
    int background =0;
    switch(cmd->command_type){
    case C_SIMPLE:
        lst = cmd->command_sequence->command_list;
        background = ((SimpleCommand*) lst->head)->background;
        break;
    case C_OR:
    case C_AND:
    case C_PIPE:
    case C_SEQUENCE:
        /*
         * last command in sequence defines whether background or
         * forground execution is specified.
         */
        lst = cmd->command_sequence->command_list;
        while (lst !=NULL){
            background = ((SimpleCommand*) lst->head)->background;
            lst=lst->tail;
        }
        break;
    case C_EMPTY:
    default:
        break;
    }
    return background;
}


int execute(Command * cmd){
    unquote_command(cmd);

    int res=0;
    List * lst=NULL;

    int execute_in_background=check_background_execution(cmd);
    switch(cmd->command_type){
    case C_EMPTY:
        break;
    case C_SIMPLE:
        res=do_execute_simple((SimpleCommand*) cmd->command_sequence->command_list->head, execute_in_background);
        fflush(stderr);
        break;

            // Neu
    case C_OR:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                SimpleCommand *cmd_s = (SimpleCommand *)lst->head;
                res = do_execute_simple(cmd_s, 0);
                if (res == 0) {
                    break;
                }
                lst = lst->tail;
            }
            break;

            // Neu
    case C_AND:
            lst = cmd->command_sequence->command_list;
            while (lst != NULL) {
                SimpleCommand *cmd_s = (SimpleCommand *)lst->head;
                res = do_execute_simple(cmd_s, 0);
                if (res != 0) {
                    break;
                }
                lst = lst->tail;
            }
            break;

            // Neu
    case C_SEQUENCE:
        lst = cmd->command_sequence->command_list;
        while (lst != NULL) {
            SimpleCommand *cmd_s = (SimpleCommand *)lst->head;
            res = do_execute_simple(cmd_s, 0);
            lst = lst->tail;
        }
        break;

    case C_PIPE:
        lst = cmd->command_sequence->command_list;

        List *temp = lst;

        // Speicher allokieren (Für PIDs, Pipes und Prognamen)
        int plen = cmd->command_sequence->command_list_len;
        pid_t *pids = malloc(sizeof(pid_t) * plen);
        int (*pipes)[2] = malloc(sizeof(int[2]) * (plen - 1));
        char **progs = malloc(sizeof(char *) * plen);

        // Erstellt pipes und prüft auf Fehler
        for (int i = 0; i < plen - 1; i++) {
            if (pipe(pipes[i]) < 0) {
                perror("pipe");
                free(pids);
                free(pipes);
                free(progs);
                return 1;
            }
        }


        int i = 0;
        while (temp != NULL) {
            SimpleCommand *cmd_s = (SimpleCommand *)temp->head;

            // Erstellt Kinderprozess
            pids[i] = fork();

            // Wenn < 0 Fehler
            if (pids[i] < 0) {
                perror("fork");
                return 1;
            }

            // Child-Prozess
            if (pids[i] == 0) {

                /*
                 * Prozessgruppe sofort selbst setzen: Kind kann schon vor dem
                 * Elternprozess exec()en und von stdin lesen. Ohne dies race't
                 * das Kind gegen das spätere setpgid()/tcsetpgrp() im Elternteil
                 * und liest ggf. als Hintergrundprozess vom Terminal (-> EIO).
                 */
                setpgid(0, i == 0 ? 0 : pids[0]);

                // Umgang mit ctr+c usw.
                signal(SIGINT, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
                signal(SIGCHLD, SIG_DFL);

                // Alle Prozesse nach dem ersten sollen von der Pipe lesen
                if (i > 0) {
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                }

                // Alle Prozesse bis auf dem letzten sollen auf die Pipe schreiben
                if (i < plen - 1) {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }

                // Die Pipes vom Child schließen, da sie ansonsten doppelt offen sind
                // (duplikate in stdin/stdout bleiben offen)
                for (int j = 0; j < plen - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                char **command = cmd_s->command_tokens;

                // Führt das Programm des Child-Prozesses aus und gibt ggf. Fehlermeldung zurück
                if (execvp(command[0], command) == -1) {
                    fprintf(stderr, "-bshell: %s : command not found \n", command[0]);
                    exit(EXIT_FAILURE);
                }
            }

            /* Prozessgruppe auch im Elternteil setzen (schließt die Race von beiden Seiten) */
            setpgid(pids[i], pids[0]);
            if (i == 0 && execute_in_background == 0) {
                /* So früh wie möglich zur Vordergrundgruppe machen */
                tcsetpgrp(fdtty, pids[0]);
            }

            /* Save prog name for statuslist */
            progs[i] = cmd_s->command_tokens[0];

            temp = temp->tail;
            i++;
        }

        // Parent
        for (int j = 0; j < plen - 1; j++) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }

        if (execute_in_background == 0) {
            int status;
            for (int j = 0; j < plen; j++) {
                int wpid = waitpid(pids[j], &status, 0);
                (void)wpid;
                statuslist_add_terminated(pids[j], pids[0], progs[j], status);

                if (j == plen - 1) {
                    if (WIFEXITED(status)) {
                        res = WEXITSTATUS(status);
                    } else if (WIFSIGNALED(status)) {
                        res = 1;
                    }
                }
            }

            tcsetpgrp(fdtty, shell_pid);
        } else {
            for (int j = 0; j < plen; j++) {
                statuslist_add(pids[j], pids[0], progs[j]);
            }
            fprintf(stderr, "PID=%d PGID=%d\n", pids[plen - 1], pids[0]);
            res = 0;
        }

        free(pids);
        free(pipes);
        free(progs);
        break;

    default:
        printf("[%s] unhandled command type [%i]\n", __func__, cmd->command_type);
        break;
    }
    return res;
}

