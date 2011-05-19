#ifndef _EXAMPLE_AUTOMATON_H
#define _EXAMPLE_AUTOMATON_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predP(void);
int predR(void);
int predQ(void);
int predD(void);
int predE(void);




int server(int argc, char *argv[]);
int client(int argc, char *argv[]);

#endif 
