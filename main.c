#include<stdio.h>
#include <stdlib.h>
#include<string.h>
#include <bits/waitflags.h>
#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

#include <unistd.h>     // fork, execvp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid, W macros
#include<signal.h>
#include <fcntl.h>


#define LSH_HIST_MAX 100   // max number of commands to remember

char *history[LSH_HIST_MAX];
int history_count = 0;

//unsafe to use 


// void *memcpy(void *dest, const void *src, size_t n)
// {
//     unsigned char *d = dest;
//     const unsigned char *s = src;

//     // Copy forward from src to dest
//     for (size_t i = 0; i < n; i++) {
//         d[i] = s[i];
//     }

//     return dest;
// }


//safe to use if d>s so it copy backward not possible in memcpy
//memcpy only have forward copying

// void *memmove(void *dest, const void *src, size_t n)
// {
//     unsigned char *d = dest;
//     const unsigned char *s = src;

//     if (d < s) {
//         // Copy forward
//         for (size_t i = 0; i < n; i++) {
//             d[i] = s[i];
//         }
//     } else {
//         // Copy backward
//         for (size_t i = n; i > 0; i--) {
//             d[i-1] = s[i-1];
//         }
//     }

//     return dest;
// }


void add_history(const char *line) {
    // ignore empty lines
    if (line == NULL || line[0] == '\0')
        return;

    // if buffer full, remove oldest entry
    if (history_count == LSH_HIST_MAX) {
        free(history[0]);
        // shift all entries left by 1
        memmove(history, history + 1, sizeof(char*) * (LSH_HIST_MAX - 1));
        history_count--;
    }

    //strdup(line) allocates new memory and copies the string into it.
    //the original line will be freed in lsh_loop();
    // history must keep its own independent copy

    history[history_count] = strdup(line);  // make a copy
    history_count++;
}



//List of built-in commands
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "history"
    
};


int(*builtin_func[])(char **)={

    &lsh_cd,
    &lsh_help,
    &lsh_exit,
    &lsh_history

};



//Function declaration for built-in commands
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_history(char **args);


int lsh_history(char **args) {
    (void)args;  // unused

    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i]);
    }
    return 1;   //To keep shell running
}




int lsh_launch(char **args) {
    int pipefd[2];
    pid_t p1, p2;

    int background=0;
    //check for background(&)

    for(int i=0;args[i]!=NULL;i++){
        if(strcmp(args[i],"&")==0){
            args[i]=NULL;
            background=1;
            break;
        }
        
    }

    char **cmd1 = args;
    char **cmd2 = NULL;

    // Find pipe(|) ,if it exists
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;        // terminate cmd1 arg list
            cmd2 = &args[i + 1];   // cmd2 starts after '|'
            break;
        }
    }

    // === NO PIPE CASE: just redirection + single command ===
    if (cmd2 == NULL) {

        pid_t pid, wpid;
        int status;

        pid = fork();

        if (pid == 0) {
            // ----- CHILD: handle redirection and exec -----

            for (int i = 0; args[i] != NULL; i++) {

                if (strcmp(args[i], "<") == 0) {
                    char *filename = args[i + 1];

                    if (filename == NULL) {
                        fprintf(stderr, "lsh: expected filename after <\n");
                        exit(EXIT_FAILURE);
                    }

                    int fd = open(filename, O_RDONLY);
                    if (fd == -1) {
                        perror("lsh: input file error");
                        exit(EXIT_FAILURE);
                    }

                    if (dup2(fd, STDIN_FILENO) == -1) {
                        perror("lsh: dup2 input error");
                        exit(EXIT_FAILURE);
                    }

                    close(fd);
                    args[i] = NULL; // stop argv here so "<" and filename aren't passed
                }
                else if (strcmp(args[i], ">") == 0) {
                    char *filename = args[i + 1];

                    if (filename == NULL) {
                        fprintf(stderr, "lsh: expected filename after >\n");
                        exit(EXIT_FAILURE);
                    }

                    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("lsh");
                        exit(EXIT_FAILURE);
                    }

                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("lsh");
                        exit(EXIT_FAILURE);
                    }

                    close(fd);
                    args[i] = NULL;
                }
            }

            
            if (execvp(args[0], args) == -1) {
                perror("lsh");
            }

            exit(EXIT_FAILURE);  // only reached if execvp fails

        } else if (pid < 0) {
            perror("lsh");
        } else {
            // ----- PARENT: wait for this one child -----

            if(!background){
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
                (void)wpid;
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            }else{
                printf("process id running : %d\n",pid);
               
            }

        }

    } else {
        // === PIPE CASE: cmd1 | cmd2 ===

        // Basic syntax check
        if (cmd2[0] == NULL) {
            fprintf(stderr, "lsh: syntax error near '|'\n");
            return 1;
        }

        // create pipe
        if (pipe(pipefd) < 0) {
            perror("lsh: pipe");
            return 1;
        }

        // First child: writer (cmd1)
        p1 = fork();
        if (p1 < 0) {
            perror("fork");
            return 1;
        }

        if (p1 == 0) {
            // Child 1 code
            close(pipefd[0]);                       // close unused read end
            dup2(pipefd[1], STDOUT_FILENO);         // stdout → pipe write
            close(pipefd[1]);

            if (execvp(cmd1[0], cmd1) == -1) {
                perror("lsh: exec cmd1");
                exit(EXIT_FAILURE);
            }
        }

        // Second child: reader (cmd2)
        p2 = fork();
        if (p2 < 0) {
            perror("fork");
            return 1;
        }

        if (p2 == 0) {
            // Child 2 code
            close(pipefd[1]);                       // close unused write end
            dup2(pipefd[0], STDIN_FILENO);          // stdin ← pipe read
            close(pipefd[0]);

            if (execvp(cmd2[0], cmd2) == -1) {
                perror("lsh: exec cmd2");
                exit(EXIT_FAILURE);
            }
        }

        // Parent: close both ends of pipe, then wait
        close(pipefd[0]);
        close(pipefd[1]);

        if(!background){

        waitpid(p1, NULL, 0);
        waitpid(p2, NULL, 0);

        }else{
            printf("Pipelining running in background\n");
        }
    }

    return 1; // keep shell running
}


int lsh_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}


//Built-in function implementation

int lsh_cd(char **args){

    if(args[1]==NULL){
        fprintf(stderr,"lsh: expected argument to \"cd \"\n");
    }else{
        
        if(chdir(args[1]) != 0){
            perror("lsh");
        }
    }
    return 1;
}


int lsh_help(char **args){

    (void)args; // avoid unused-parameter warning
    
    int i;
    
  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;

}


int lsh_exit(char **args)
{
  (void)args;
  return 0;
}



char *lsh_read_line(void){

    int buffsize=LSH_RL_BUFSIZE;

    //number of characters
    int position=0;

    //It stores the exact characters the user types, in order.
    char *buffer=malloc(sizeof(char) * buffsize);

    //getchar() returns an int, not a char.EOF is -1 so not able to stored in char
    int c;

    if(!buffer){
        fprintf(stderr,"lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){

        c=getchar();

        //while comparing character literal is converted to int.(even character==character)
        if(c==EOF || c=='\n'){
            buffer[position]='\0';
            return buffer;
        }else{
            buffer[position]=c;
        }

        position++;

        if(position >= buffsize){
            buffsize+=LSH_RL_BUFSIZE;
            // realloc() may need to allocate a new larger block elsewhere,
            // copy the old data there, and free the old block

            buffer=realloc(buffer,buffsize);

            if(!buffer){
                fprintf(stderr,"lsh:allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

    }

    return buffer;

}



char **lsh_split_line(char *line){

    int bufsize=LSH_TOK_BUFSIZE,position=0;

    char ** tokens=malloc(bufsize * sizeof(char*));

    char *token;

    if(!tokens){
        fprintf(stderr,"lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    //strtok splits the string line using delimiters in LSH_TOK_DELIM ("\t\r\n\a").
    token=strtok(line,LSH_TOK_DELIM);
    // printf("token-----------> %s",token);

    while(token!=NULL){
        tokens[position] = token;
        position++;

        if(position>= bufsize){
            bufsize+=LSH_TOK_BUFSIZE;
            tokens=realloc(tokens,bufsize* sizeof(char*));

            if(!tokens){
                fprintf(stderr,"lsh:allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        //It will return next token and If not found it will return NULL
        token=strtok(NULL,LSH_TOK_DELIM);
    }

    tokens[position]=NULL;
    return tokens;

}


void lsh_loop(){

    char *line;
    char ** args;
    int status;
    

    do{

        printf("> ");
        fflush(stdout);
        //It will store the raw input from the user.
        line=lsh_read_line();
        add_history(line);

     

        //This will store the tokens (arguments) after splitting the line
        args=lsh_split_line(line);


       // Stores the return value of lsh_execute(args).
       //status = 1 → keep running the shell
        //status = 0 → exit the shell
        status=lsh_execute(args);

        // printf("status-----> %d",status);

        //to free the memory in heap
        free(line);
        free(args);
        

    }while(status);

}





int main(int argc, char **argv){

    //argc ->argument counter ,it will count number of arguments
    //argv ->argument vector ,it will take total number of arguments into vector, argv[0]="" argv[1]=""

    (void)argc;
    (void)argv;
    lsh_loop();
    
    return EXIT_SUCCESS;
}