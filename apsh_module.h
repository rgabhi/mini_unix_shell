    #ifndef APSH_MODULE_H
    #define APSH_MODULE_H

    // prototypes def
    int apsh_cd(char **args);
    void handle_sigint(int sig);
    void handle_sigchld(int sig);
    int apsh_exit(char **args);
    void execute_pipeline(char **left_args, char **right_args);

    // struct def
    
    #endif 