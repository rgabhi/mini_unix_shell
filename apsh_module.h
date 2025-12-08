    #ifndef APSH_MODULE_H
    #define APSH_MODULE_H

    // prototypes def
    int apsh_cd(char **args);
    void handle_sigint(int sig);
    void handle_sigchld(int sig);
    int apsh_exit(char **args);
    void execute_pipeline(char **left_args, char **right_args);
    int apsh_export(char **args);
    int check_background(char ** args);
    void add_prompt();


    // struct def
    typedef struct Node {
        char *cmd;
        struct Node *prev;
        struct Node *next;
    } Node;

    typedef struct {
        Node *head;      // Most recently used
        Node *tail;      // Least recently used
        int capacity;    // Max number of commands
        int size;        // Current number of commands
    } LRUCache;

    // Create an LRU history cache
    LRUCache *lru_create(int capacity);
    // Free the LRU cache
    void lru_free(LRUCache *cache);
    // Add a command to the cache (moves existing ones to front)
    void lru_put(LRUCache *cache, const char *cmd);
    // Print history from MRU to LRU
    int lru_print(LRUCache *cache);


  

    #endif 