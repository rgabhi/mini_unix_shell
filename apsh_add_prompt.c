#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h> //waitpid
#include<signal.h> // for signal, SIGINT
#include <unistd.h> 


#include "apsh_module.h"

void add_prompt(){
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\033[1;34m%s\033[0m \033[1;32mAP_SHELL\033[0m\033[1;36m >> \033[0m", cwd);
}