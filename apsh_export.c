#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>
#include <unistd.h>  // fork, execvp
#include <sys/wait.h> //waitpid
#include<fcntl.h>   // for open(),O_CREAT ,etc.....
#include<signal.h> // for signal, SIGINT


int apsh_export(char **args) {

    if (args[1] == NULL) {
        fprintf(stderr, "apsh: export: expected KEY=VALUE\n");
        return 1;
    }

    // format must be KEY=VALUE
    char *arg = args[1];
    char *equal_sign = strchr(arg, '=');

   
    if (!equal_sign) {
        fprintf(stderr, "apsh: export: invalid format, expected KEY=VALUE\n");
        return 1;
    }

    // Split into key and value
    *equal_sign = '\0';
    char *key = arg;
    char *value = equal_sign + 1;



//     Every Unix/Linux process has an environment block,
//     which is stored in the process’s user-space memory, inside its address space, 
//     typically on the stack area at startup and later managed in the heap.

//      The environment is accessible through a global variable:extern char **environ

//      Copied on fork()
//      Reinitialized on exec()

//      1->overwrite , 0->no overwrite
 
//      setenv allocates new memory and copies the strings.

    if (setenv(key, value, 1) != 0) {
        perror("apsh: export");
    }

    return 1;
}
