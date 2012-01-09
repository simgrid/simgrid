#ifndef _EXAMPLE_LIVENESS_WITH_CYCLE_H
#define _EXAMPLE_LIVENESS_WITH_CYCLE_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predP(void);
int predQ(void);

int coordinator(int argc, char *argv[]);
int client(int argc, char *argv[]);

#endif 
