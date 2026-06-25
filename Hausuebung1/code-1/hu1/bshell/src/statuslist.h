#ifndef STATUSLIST_H

#define STATUSLIST_H

typedef struct ProcessInfo {
    pid_t pid;
    pid_t pgid;
    int status;
    int finished;
    
    char name[256];

    struct ProcessInfo *next;
} ProcessInfo;

ProcessInfo* process_add(pid_t pid, pid_t pgid, const char * name);
void process_update(pid_t pid, int status);
void print_status(void);

#endif /* end of include guard: STATUSLIST_H */
