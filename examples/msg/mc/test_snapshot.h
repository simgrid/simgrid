#ifndef _TEST_SNAPSHOT_H
#define _TEST_SNAPSHOT_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predCS(void);
int predR(void);

int coordinator(int argc, char *argv[]);
int client(int argc, char *argv[]);

void check(void);

#endif 
