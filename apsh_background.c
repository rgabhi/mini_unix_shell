#include<stdio.h>
#include "apsh_module.h"
#include<string.h>

int check_background(char ** args){

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

   return is_background;

}