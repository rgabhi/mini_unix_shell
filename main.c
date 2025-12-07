#include<stdio.h>
#include <stdlib.h>
#include<string.h>



int main(int argc, char **argv){

    //argc ->argument counter ,it will count number of arguments
    //argv ->argument vector ,it will take total number of arguments into vector, argv[0]="" argv[1]=""

    (void)argc;
    (void)argv;
    lsh_loop();
    
    return EXIT_SUCCESS;
}