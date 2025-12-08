#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // fork, execvp
#include <sys/wait.h> //waitpid
#include "apsh_module.h"

void execute_pipeline(char **left_args, char **right_args){

    // int is_left_background=check_background(left_args);
    int is_right_background=check_background(right_args);

    int pfd[2];
    pid_t p1, p2;
    if (pipe(pfd) < 0){
        perror("pipe");
        return;
    }
    p1 = fork();
    if(p1 == 0){
        // first child, write to pipe
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[0]);
        close(pfd[1]);
        if(execvp(left_args[0], left_args) == -1){
            perror("apsh");
            exit(EXIT_FAILURE);
        }
    }

    p2 = fork();
    if(p2 == 0){
        // second child, read from pipe
        dup2(pfd[0], STDIN_FILENO);
        close(pfd[0]);
        close(pfd[1]);
        if(execvp(right_args[0], right_args) == -1){
            perror("apsh");
            exit(EXIT_FAILURE);
        }
    }

    //added by me   -------------- >>

    close(pfd[0]);
    close(pfd[1]);
    // printf("runigng in background\n");

    if(!is_right_background ){

        waitpid(p1, NULL, 0);
        waitpid(p2, NULL, 0);

    }

 
}