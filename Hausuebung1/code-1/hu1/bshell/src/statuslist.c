#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "statuslist.h"

static ProcessInfo *head = NULL;

ProcessInfo* process_add(pid_t pid, pid_t pgid, const char * name) {
    ProcessInfo *p = malloc(sizeof(ProcessInfo));
    p->pid = pid;
    p->pgid = pgid;

    strncpy(p->name, name, sizeof(p->name)-1);
    p->name[sizeof(p->name)-1] = '\0';
    p->status = 0;
    p->finished = 0;

    p->next = head;
    head = p;

    return p;
}

void process_update(pid_t pid, int status) {
    ProcessInfo *p = head;
    while (p) {
        if (p->pid == pid) {
            p->finished = 1;
            p->status = status;
            return;
        }

        p = p->next;
    }

    // Process not found, use Dummy Entry
    process_update(-1, status);
}

void refersh_status(void) {
    ProcessInfo *p = head;

    while(p) {
        if (!p->finished) {
            int status;
            pid_t r = waitpid(p->pid, &status, WNOHANG);

            if(r > 0) {
                p->finished = 1;
                p->status=status;
            } else if(r == -1 && errno != ECHILD) {
                perror("waitpid");
            }
        }

        p = p-> next;
    }
}

void print_status(void) {
    refersh_status();
    printf("PID\tPGID\tSTATUS\t PROG\n");
    ProcessInfo *p = head;
    ProcessInfo *prev = NULL;
    while (p) {
        if(!p -> finished) {
            printf("%d\t%d\trunning\t   %s\n", p->pid, p->pgid, p->name);
            prev = p;
            p = p->next;
        } else {
            if(WIFEXITED(p->status)) {
                printf("%d\t%d\texit(%d)\t  %s\n", p->pid, p->pgid, WEXITSTATUS(p->status), p->name);
            } else if(WIFSIGNALED(p->status)) {
                printf("%d\t%d\tsignal(%d)\t    %s\n", p->pid, p->pgid, WTERMSIG(p->status), p->name);
            } else {
                printf("%d\t%d\tunknown\t   %s\n", p->pid, p->pgid, p->name);
            }

            ProcessInfo *tmp = p;

            if (prev) {
                prev->next = p->next;
            } else {
                head = p->next;
            }

            p = p->next;

            free(tmp);
        }
    }
}

