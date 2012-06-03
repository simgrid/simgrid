#ifndef _CENTRALIZED_LIVENESS_H
#define _CENTRALIZED_LIVENESS_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predCS(void);

int coordinator(int argc, char *argv[]);
int client(int argc, char *argv[]);

#endif 
