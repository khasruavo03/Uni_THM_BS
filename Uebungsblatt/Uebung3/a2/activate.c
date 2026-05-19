#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int delimiter = -1;

    for (int i = 1; i < argc; i++) {
        if(strcmp(argv[i],";") == 0) {
            delimiter = i;
            break;
        }
    }

    if(delimiter == -1) {
        fprintf(stderr, "Delimiter ';' not found\n");
        exit(1);
    }
    
    argv[delimiter] = NULL;
    char **prog1 = &argv[1];

    char **prog2 = &argv[delimiter + 1];

    pid_t pid1 = fork();

    if(pid1 == 0){
        execvp(prog1[0], prog1);
        perror("execvp failed");
        exit(1);
    }

    pid_t pid2 = fork();

    if(pid2 == 0){
        execvp(prog2[0], prog2);
        perror("execvp failed");
        exit(1);
    }

    /* Elternprozess wartet */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}