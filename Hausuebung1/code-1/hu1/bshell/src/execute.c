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
    pid = fork();
    if (pid==0){
        /* child */
        signal(SIGINT, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        /* if(in_fd >= 0) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }

        if (out_fd >= 0) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
            */

        /*
         * handle redirections here
         */
        if (cmd_s->redirections != NULL){
            List* lst = cmd_s->redirections; 
            while(lst != NULL) {
                Redirection *redir = (Redirection *) lst->head;
                int fd;

                if(redir->r_mode == M_READ) {
                    // Datei öffnen und nur lesen
                    fd = open(redir->u.r_file, O_RDONLY);
                    if (fd >= 0) {
                        dup2(fd, STDIN_FILENO); // Kanal 0
                    } else {
                        perror("bshell:Redirections open or dup2 failed");
                        // exit(EXIT_FAILURE);
                    }
                } else if (redir->r_mode == M_WRITE) {
                    // Datei öffnen und Schreiben, Erstellen, falls nichts vorhanden oder Kürzen, falls vorhanden
                    fd = open(redir->u.r_file, O_WRONLY | O_CREAT| O_TRUNC, 0644);
                    if (fd >= 0) {
                        dup2(fd, STDOUT_FILENO); // Kanal 1
                    } else {
                        perror("bshell:Redirections open or dup2 failed");
                        // exit(EXIT_FAILURE);
                    }
                } else if (redir->r_mode == M_APPEND) {
                    // Datei öffnen und Schreiben, Erstellen, falls nichts vorhanden oder Anfügen
                    fd = open(redir->u.r_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd >= 0) {
                        dup2(fd, STDOUT_FILENO); // Kanal 1
                    } else {
                        perror("bshell:Redirections open or dup2 failed");
                        // exit(EXIT_FAILURE);
                    }
                }

                /* // Fehlerkontrolle
                if (fd < 0) {
                    perror("bshell:Redirections open failed");
                    exit(EXIT_FAILURE);
                } */

                //Den ungenutzen, alten Descriptor zu schließen
                close(fd);

                lst = lst->tail; // Nächste Umlenkung
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
        setpgid(pid, pid); // Setzt neue Prozessgruppe
        if (background == 0) {
            /* wait only if no background process */
            
            // Übergibt Terminalkontrolle an Child-Prozess
            tcsetpgrp(fdtty, pid);

            // Speicher Exit-Status
            int status;

            // Wartet bis Child fertig ist
            wpid= waitpid(pid, &status, 0);

            // Gibt Terminalkontrolle zurück an Shell
            tcsetpgrp(fdtty, shell_pid);

            // Prüft, ob Prozess normal beendet würde
            // Wenn ja? - Liefert Exit-Code zurück (Was wichtig für AND und OR ist)
            // Wenn nein? - Fehlerfall
            return WIFEXITED(status)? WEXITSTATUS(status) : 1;
        }
    }

    return 0;
}


static int do_execute_simple(SimpleCommand *cmd_s, int background){
    if (cmd_s==NULL){
        return 0;
    }

    // Exit Command
    if (strcmp(cmd_s->command_tokens[0],"exit")==0){
        exit(0);

    // Change Directory
    } else if (strcmp(cmd_s->command_tokens[0], "cd") == 0) {
        char* path = cmd_s->command_tokens[1];
        // HOME Directory
        if (path == NULL || strcmp(path, "~") == 0) {
            path = getenv("HOME");
        }

        // Fehlermeldung 
        if (chdir(path) != 0) {
            perror("cd");
            return 1;
        }

        return 0;

/* do not modify this */
#ifndef NOLIBREADLINE
    // History
    } else if (strcmp(cmd_s->command_tokens[0],"hist")==0){
        return builtin_hist(cmd_s->command_tokens);
    
#endif /* NOLIBREADLINE */
    } else {
        return execute_fork(cmd_s, background);
    }
    fprintf(stderr, "This should never happen!\n");
    exit(1);
}

static int execute_pipeline(Command *cmd) {
    List *lst = cmd->command_sequence->command_list;

    int in_fd = -1; //liest aus der vorherige Pipe
    pid_t pids[128];
    int pid_count = 0;

    while(lst != NULL) {
        SimpleCommand *simpleCmd = (SimpleCommand*) lst->head;
        int fd[2] = {-1, -1};

        if (lst->tail != NULL && pipe(fd) < 0) {
            perror("pipe");
            return 1;
        }
        
        pid_t pid = fork();
        
        //KIndprozess
        if (pid == 0) {
            if (in_fd >= 0) {
                dup2(in_fd, STDIN_FILENO);
            } 
            
            if (fd[1] >= 0) {
                dup2(fd[1], STDOUT_FILENO);
            }

            // Was nicht gebraucht wird, wird geschlossen
            if (in_fd >= 0) {
                close(in_fd);
            } 
            if (fd[0] >= 0) {
                close(fd[0]);
            } 
            if (fd[1] >= 0) {
                close(fd[1]);
            }

            execvp(simpleCmd->command_tokens[0], simpleCmd->command_tokens);

            perror("execvp");
            exit(EXIT_FAILURE);
        
        // Parent
        } else if (pid > 0) {

            pids[pid_count++] = pid;

            if (in_fd >= 0) {
                close(in_fd);
            }
            if (fd[1] >= 0) {
                close(fd[1]);
            }

            in_fd = fd[0];

        } else {
            perror("fork");
        }

        lst = lst->tail;
    }

    if (in_fd >= 0) {
        close(in_fd);
    } 

    int status;

    for(int i = 0; i < pid_count; i++) {
        waitpid(pids[i], &status, 0);
    }

    return WIFEXITED(status)? WEXITSTATUS(status) : 1;
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
    
    // Wenn fehlgeschlagen
    case C_OR:
        lst = cmd->command_sequence->command_list;

        while (lst != NULL) {
            SimpleCommand *simpleCmd = (SimpleCommand*) lst->head;
            res = do_execute_simple(simpleCmd, 0);

            if (res == 0) {
                break;
            }

            lst = lst->tail;
        }
        break;

    // Wenn erfolgreich war
    case C_AND:
        lst = cmd->command_sequence->command_list;

        while (lst != NULL) {
            SimpleCommand *simpleCmd = (SimpleCommand*) lst->head;
            res = do_execute_simple(simpleCmd, 0);

            if (res != 0) {
                break;
            }

            lst = lst-> tail;
        }

        break;
    case C_SEQUENCE:
        //Iteration through the command list!
        lst = cmd->command_sequence->command_list;
        
        while (lst !=NULL){
            SimpleCommand* simpleCmd = (SimpleCommand*) lst->head;
            // Nicht: res = do_execute_simple(simpleCmd, 0);
            res = do_execute_simple(simpleCmd, simpleCmd->background);
            lst=lst->tail;
    
        }

        break;
    case C_PIPE:
        res = execute_pipeline(cmd);
        /* int in_fd = -1;
        List *lst = cmd->command_sequence->command_list;

        while (lst != NULL) {
            int fd[2] = {-1, -1};

            // 
            if (lst->tail != NULL) { 
                pipe(fd);
            }
            SimpleCommand *simpleCmd = (SimpleCommand*) lst->head;
            res = execute_fork(simpleCmd, 0, in_fd, fd[1]);

            if(in_fd >= 0) {
                close(in_fd);
            }

            if(fd[1] >= 0) {
                close(fd[1]);
            }

            in_fd = fd[0];
            lst = lst->tail;
            
        }*/
        break;
    default:
        printf("[%s] unhandled command type [%i]\n", __func__, cmd->command_type);
        break;
    }
    return res;
}

