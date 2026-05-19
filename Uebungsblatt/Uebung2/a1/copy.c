#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) 
{
    if (argc != 3) 
    {
        fprintf(stderr, "Usage: %s <INPUT> <OUTPUT> \n", argv[0]);
        return 1;
    }

    int in = open(argv[1], O_RDONLY);
    
    if (in < 0) 
    {
        perror("open input");
        return 1;
    }

    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (out < 0) 
    {
        perror("open input");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t n;

    while ((n = read(in, buffer, BUFFER_SIZE))) 
    {
        write(out, buffer, n);
    }

    close(in);
    close(out);

    return 0;   
}