#include <string.h>
#include <stdio.h>
#include <gras.h>

void parse_log_opt(int argc, char **argv,const char *deft);

void parse_log_opt(int argc, char **argv,const char *deft) {
   char *opt;
   gras_error_t errcode;
   
   if (argc > 1 && !strncmp(argv[1],"--gras-log=",strlen("--gras-log="))) {
    opt=strchr(argv[1],'=');
    opt++;
    TRYFAIL(gras_log_control_set(opt));
  } else {
    TRYFAIL(gras_log_control_set(deft));
  }
}

