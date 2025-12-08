#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>
#include <unistd.h>  // fork, execvp
#include <sys/wait.h> //waitpid

#define TOKEN_BUFF_SZ 64
#define TOKEN_DELIMS " \t\r\n\a"

int launch(char **args){
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid == 0){
        // child process
        if(execvp(args[0], args) == -1){
            //print error if cmd not found
            perror("apsh");
        }
        // kill child if exec fails
        exit(EXIT_FAILURE);
    }
    else if(pid < 0){
        // error forking
        perror("apsh");
    }
    else{
        wpid = waitpid(pid, &status, WUNTRACED);
        while(!WIFEXITED(status) && !WIFSIGNALED(status)){
            wpid = waitpid(pid, &status, WUNTRACED);
        }
    }
    return 1; // 1 to keep shell running 
}

char **tokenize_input(char *line){
    int bufsize = TOKEN_BUFF_SZ;
    int position = 0;
    // allocate buffer
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    
    if(!tokens){
        // write to file stream
        fprintf(stderr, "apsh : allocation error\n");
        exit(EXIT_FAILURE);
    }

    //2. get first token
    token = strtok(line, TOKEN_DELIMS);
    while(token != NULL){
        tokens[position] = token;
        position++;

        // if exceed buffer size, realloc
        if(position >= bufsize){
            bufsize += TOKEN_BUFF_SZ;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens){
                fprintf(stderr, "apsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        // 3. get next token- pass null
        token = strtok(NULL, TOKEN_DELIMS);

    }

    // 4. NULL terminate arr
    tokens[position] = NULL;
    return tokens;

}


int execute(char **args){
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
   // if not a built-in, run as external process
   return launch(args);
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

    while(status){
        // 1.
        printf("||AP_SHELL||>>> ");

        // 2. read line from stdin
        read = getline(&line, &len, stdin);
        
        // 3. get args
        args = tokenize_input(line);
        if(args[0] != NULL){
            status = execute(args);
        }
        free(args);
    }
    free(line);
    return 0;
}