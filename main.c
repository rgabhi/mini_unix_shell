#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include<string.h>
#include <unistd.h>  // fork, execvp
#include <sys/wait.h> //waitpid
#include<fcntl.h>   // for open(),O_CREAT ,etc.....

#define TOKEN_BUFF_SZ 64
#define TOKEN_DELIMS " \t\r\n\a"


// commit2
int launch(char **args) {
   pid_t pid;
   int status;
   int is_background = 0;


   // 1. Check if the last argument is '&'
   int i;
   for (i = 0; args[i] != NULL; i++) {
       // Just finding the end...
   }
   // 'i' is now the count of arguments. Check the last one (i-1).
   if (i > 0 && strcmp(args[i-1], "&") == 0) {
       is_background = 1;
       args[i-1] = NULL; // Remove '&' from the list
   }


   pid = fork();
   if (pid == 0) {
       // --- CHILD PROCESS ---
      
       // commit 7
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
       // end commit 7


       if (execvp(args[0], args) == -1) {
           perror("lsh");
       }
       exit(EXIT_FAILURE);
      
   } else if (pid < 0) {
       perror("lsh");
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
       }
   }


   return 1;
}








// commit 1
char **tokenize_input(char *line){
   int bufsize = TOKEN_BUFF_SZ;
   int position = 0;
   // allocate buffer
   char **tokens = malloc(bufsize * sizeof(char*));
   char *p = line;


  
   if(!tokens){
       // write to file stream
       fprintf(stderr, "lsh : allocation error\n");
       exit(EXIT_FAILURE);
   }


   //2. get first token
   while(*p != NULL){


       // 1. skip leading white space
       while(*p == ' ' || *p == '\t' || *p == '\n')p++;
      
       if(*p == '\0')break; // end of line


       // handle quoted strings
       if(*p == '"'){
           p++; // skip opening quote
           tokens[position] = p;
           position++;
          
           // scan closing quote
           while(*p && *p != '"'){
               p++;
           }


           if(*p == '"'){
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
           if(*p != NULL){
               *p = '\0';
               p++;
           }
       }


       // 4. if exceed buffer size, realloc
       if(position >= bufsize){
           bufsize += TOKEN_BUFF_SZ;
           tokens = realloc(tokens, bufsize * sizeof(char*));
           if(!tokens){
               fprintf(stderr, "lsh: allocation error\n");
               exit(EXIT_FAILURE);
           }
       }


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