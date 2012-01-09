#ifndef _BUGGED2_LIVENESS_H
#define _BUGGED2_LIVENESS_H

int yyparse(void);
int yywrap(void);
int yylex(void);

int predPready(void);
int predCready(void);
int predConsume(void);
int predProduce(void);

int coordinator(int argc, char *argv[]);
int producer(int argc, char *argv[]);
int consumer(int argc, char *argv[]);

#endif 
