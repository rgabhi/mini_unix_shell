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
            perror("lsh");
        }
        // kill child if exec fails
        exit(EXIT_FAILURE);
    }
    else if(pid < 0){
        // error forking
        perror("lsh");
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
        fprintf(stderr, "lsh : allocation error\n");
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
                fprintf(stderr, "lsh: allocation error\n");
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
        printf("ag_mini_shell> ");

        // 2. read line from stdin
        read = getline(&line, &len, stdin);
        
        // 3. get args
        args = tokenize_input(line);
        if(args[0] != NULL){
            status = launch(args);
        }
        free(args);
    }
    free(line);
    return 0;
}