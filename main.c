#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>
#include <unistd.h>  // fork, execvp
#include <sys/wait.h> //waitpid
#include<fcntl.h>   // for open(),O_CREAT ,etc.....
#include<signal.h> // for signal, SIGINT
#include<errno.h>  // for cmd not found

#include "apsh_module.h"

#define TOKEN_BUFF_SZ 64
#define TOKEN_DELIMS " \t\r\n\a"


// LAUNCH METHOD
int launch(char **args) {
   pid_t pid;
   int status;
   
   int is_background=check_background(args);

   pid = fork();
   if (pid == 0) {
       // --- CHILD PROCESS ---
      
       // check for redirection
       for (int j = 0; args[j] != NULL; j++) {
           if (strcmp(args[j], ">") == 0) {
               args[j] = NULL; // truncate
               char *filename = args[j+1];
               if(filename == NULL){
                   fprintf(stderr, "no output file given\n");
                   exit(EXIT_FAILURE);
               }
               int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
               // open file
               if(fd == -1){
                   perror("open");
                   exit(EXIT_FAILURE);
               }
               // replace
               dup2(fd, STDOUT_FILENO);
               close(fd);
           }
           else if (strcmp(args[j], "<") == 0) {
               args[j] = NULL;
               char *filename = args[j+1];
               if(filename == NULL){
                   fprintf(stderr, "no input file given\n");
                   exit(EXIT_FAILURE);
               }
               // open file: read only
               int fd = open(filename, O_RDONLY);
               if(fd == -1){
                   perror("open");
                   exit(EXIT_FAILURE);
               }
               //replace
               dup2(fd, STDIN_FILENO);
               close(fd);
           }
       }


       if (execvp(args[0], args) == -1) {
            if(errno == ENOENT){
               fprintf(stderr, "apsh: command not found: %s\n", args[0]);
            }
            else{
               perror("apsh");
            }

       }
       exit(EXIT_FAILURE);
      
   } else if (pid < 0) {
       perror("apsh");
   } else {
       // --- PARENT PROCESS ---
      
       if (is_background) {
           // Background: Print PID and continue WITHOUT waiting
           printf("[Process %d started in background]\n", pid);
       } else {
           // Foreground: Wait for the process to finish
           do {
               waitpid(pid, &status, WUNTRACED);
           } while (!WIFEXITED(status) && !WIFSIGNALED(status));

           // If the child exited normally but with a non-zero status (failure)
           if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
               return 2; // Return 2 to signal command failure (but keep shell loop alive)
           }
       }
   }


   return 1;
}






char **tokenize_input(char *line){
   int bufsize = TOKEN_BUFF_SZ;
   int position = 0;
   // allocate buffer
   char **tokens = malloc(bufsize * sizeof(char*));
   char *p = line;


  
   if(!tokens){
       // write to file stream
       fprintf(stderr, "apsh : allocation error\n");
       exit(EXIT_FAILURE);
   }


   //2. get first token
   while(*p){


       // 1. skip leading white space
       while(*p == ' ' || *p == '\t' || *p == '\n')p++;
      
       if(*p == '\0')break; // end of line


       // handle quoted strings
       if(*p == '"' || *p == '\''){
        char quote = *p;
           p++; // skip opening quote
           tokens[position] = p;
           position++;
          
           // scan closing quote
           while(*p && *p != quote){
               p++;
           }


           if(*p == quote){
               *p = '\0'; //terminate token
               p++;
           }
       }
       else{
           // handle normal words
           tokens[position] = p;
           position++;


           // search next whitespace
           while(*p && *p != ' ' && *p != '\t' && *p != '\n'){
               p++;
           }
           // if space terminate token
           if(*p){
               *p = '\0';
               p++;
           }
       }


       // 4. if exceed buffer size, realloc
       if(position >= bufsize){
           bufsize += TOKEN_BUFF_SZ;
           tokens = realloc(tokens, bufsize * sizeof(char*));
           if(!tokens){
               fprintf(stderr, "apsh: allocation error\n");
               exit(EXIT_FAILURE);
           }
       }


   }


   // 4. NULL terminate arr
   tokens[position] = NULL;
   return tokens;


}




int execute(char **args, LRUCache *cache){
    if(args[0] == NULL){
        // empty command
        return 1;
    }
    // check pipeline |
    for(int i = 0; args[i] != NULL; i++){
        if(strcmp(args[i], "|") == 0){
            args[i] = NULL; // split
            execute_pipeline(args, &args[i + 1]);
            return 1;
        }
    }


    if(strcmp(args[0], "cd") == 0){
        return apsh_cd(args);
    }
    if(strcmp(args[0], "exit") == 0){
        return apsh_exit(args);
    }
    if(strcmp(args[0], "export") == 0){
        return apsh_export(args);
    }
    if (strcmp(args[0], "history") == 0){
        return lru_print(cache);
    }

   // if not a built-in, run as external process
   return launch(args);
}

int find_and_and(char **args) {
    if (!args) return -1;
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&&") == 0) {
            return i;   // index of &&
        }
    }
    return -1;  // not found
}

int apsh_execute_with_and(char **args, LRUCache *cache) {
    int idx = find_and_and(args);

    if (idx == -1) {
        // No && → just run normally
         return execute(args, cache);
       
    }

    // We have: cmd1 && cmd2
    // args looks like: [cmd1, ..., "&&", cmd2, ..., NULL]

    // Split into two NULL-terminated command arrays
    args[idx] = NULL;              // terminate first command
    char **args1 = args;           // left side
    char **args2 = &args[idx + 1]; // right side begins after "&&"

    // printf("-=--------> %s",args2[0]);

    // 1) Run first command
    int status1 = execute(args1, cache);

    // IMPORTANT: status1 here should be the *exit code* of the command.
    // Make sure apsh_execute_single returns WEXITSTATUS of the child.

    if (status1 == 1) {
        // success → run second command
          return apsh_execute_with_and(args2, cache);
         
    } else {
        // failure → do NOT run second command
        return status1;
    }
}




int main(){
    // ptr to line buffer 
    char *line = NULL;
    // size of line
    size_t len = 0;
    // num chars read
    ssize_t read;
    char **args;
    int status = 1;
    LRUCache *lru_cache = lru_create(20);
    

    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld); // Handle Zombies (Background processes)

    printf(
    "\033[1;31m"  // Red skull
    "      .----.   @   @\n"
    "     / .-\"-.`.  \\v/\n"
    "     | | '\\ \\ \\_/ )\n"
    "   ,-' \\ `-.' /  /\n"
    "  '---`----'----'\n"
    "\033[0m"
    "\n\033[1;32m        AP_SHELL\033[0m\n\n"  // Green AP_SHELL centered
    );


    while(status){
        // 1.
        // printf("||AP_SHELL||>> ");
        add_prompt();
        

        //added by me
        // fflush(stdout);

        // 2. read line from stdin
        read = getline(&line, &len, stdin);
        
        // handle Ctrl-D - exit
        if(read == -1){
            if (feof(stdin)){
                printf("\n");
                break;
            }
            else{
                // handle Ctrl-C
                clearerr(stdin);
                continue;
            }
        }
        
        // 2.a add to history
        lru_put(lru_cache, line);

        // 3. get args
        args = tokenize_input(line);
        if(args[0] != NULL){
            status = apsh_execute_with_and(args, lru_cache);
        }
        free(args);
    }
    lru_free(lru_cache);
    free(line);
    return 0;
}