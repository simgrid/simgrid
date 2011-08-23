#ifndef _EXAMPLE2_LIVENESS_WITHOUT_CYCLE_H
#define _EXAMPLE2_LIVENESS_WITHOUT_CYCLE_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predP(void);
int predQ(void);

int server(int argc, char **argv);
int client(int argc, char **argv);

#endif 
