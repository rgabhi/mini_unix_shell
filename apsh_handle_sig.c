#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h> //waitpid
#include<signal.h> // for signal, SIGINT
#include "apsh_module.h"

// function to handle Ctrl-C
void handle_sigint(int sig){
    printf("\n");
    fflush(stdout);
}

// Handler for SIGCHLD to clean up zombie processes
void handle_sigchld(int sig) {
    // waitpid(-1) means "wait for ANY child"
    // WNOHANG means "don't block if no child has exited yet"
    while (waitpid(-1, NULL, WNOHANG) > 0);
    fflush(stdout);
}
