#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>

#define TOKEN_BUFF_SZ 64
#define LSH_TOK_DELIM " \t\r\n\a"

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
    token = strtok(line, LSH_TOK_DELIM);
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
        token = strtok(NULL, LSH_TOK_DELIM);

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
        args = split_line(line);
        if(args[0] != NULL){
            status = launch(args);
        }
        free(args);
    }
    free(line);
    return 0;
}