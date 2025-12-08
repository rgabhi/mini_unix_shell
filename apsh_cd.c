#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "apsh_module.h"
int apsh_cd(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "apsh: expected argument to \"cd\"\n");
    }
    else{
        if(chdir(args[1]) != 0){
            // dir doesn't exist
            perror("apsh");
        }
    }
    return 1;
}