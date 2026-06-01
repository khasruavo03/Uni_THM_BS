#ifndef STATUSLIST_H

#define STATUSLIST_H

typedef struct ProcessInfo {
    pid_t pid;
    pid_t pgid;
    int status;
    int finished;
    char name[256];

    struct ProcessInfo *next;
}

status_add(pid_t pid, pid_t pgid, const char * name);

#endif /* end of include guard: STATUSLIST_H */
