#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "statuslist.h"


void status_add(pid_t pid, pid_t pgid, const char * name) {
    ProcessInfo *p = malloc(sizeof(ProcessInfo));
    p->pid = pid;
    p->pgid = pgid;
    p->status = 0;
    p->finished = 0;
}

status_print() {}

