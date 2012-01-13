#ifndef _BUGGED1_LIVENESS_H
#define _BUGGED1_LIVENESS_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predR(void);
int predCS(void);

int coordinator(int argc, char *argv[]);
int client(int argc, char *argv[]);

#endif 
