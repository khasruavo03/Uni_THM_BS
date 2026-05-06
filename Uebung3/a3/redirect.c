#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {

    if(argc < 4){
        fprintf(stderr,"Usage: redirect <OUTPUT> <ERROR> <PROGRAM> [PARAM...]\n");
        exit(1);
    }

    int fd;

    /* stdout umleiten */
    if(strcmp(argv[1], "-") != 0){
        fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(fd < 0){
            perror("open output");
            exit(1);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    /* stderr umleiten */
    if(strcmp(argv[2], "-") != 0){
        fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(fd < 0){
            perror("open error");
            exit(1);
        }

        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    /* Programm starten */
    execvp(argv[3], &argv[3]);

    perror("execvp failed");
    exit(1);
}