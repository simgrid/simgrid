    #define _GNU_SOURCE
    #include <stdio.h>
    int main(void){
      FILE * fp;
      char * line = NULL;
      size_t len = 0;
      getline(&line, &len, fp);
    }
