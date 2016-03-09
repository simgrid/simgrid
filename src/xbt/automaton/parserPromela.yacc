/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

%{
#include "simgrid_config.h"
#if !HAVE_UNISTD_H
#define YY_NO_UNISTD_H /* hello Windows */
#endif

#include "automaton_lexer.yy.c"
#include <xbt/automaton.h>

void yyerror(const char *s);

%}

%union{
  double real;
  int integer;
  char* string;
  xbt_automaton_exp_label_t label;
}

%token NEVER
%token IF
%token FI
%token IMPLIES
%token GOTO
%token AND
%token OR
%token NOT
%token LEFT_PAR
%token RIGHT_PAR
%token CASE
%token COLON
%token SEMI_COLON
%token CASE_TRUE
%token LEFT_BRACE
%token RIGHT_BRACE
%token <integer> LITT_ENT
%token <string> LITT_CHAINE
%token <real> LITT_REEL
%token <string> ID

%type <label> exp;

%start automaton

%left AND OR
%nonassoc NOT

%%

automaton : NEVER LEFT_BRACE stateseq RIGHT_BRACE 
          ;

stateseq : 
         | ID COLON { new_state($1, 1);} IF option FI SEMI_COLON stateseq 
         ;

option :
       | CASE exp IMPLIES GOTO ID option { new_transition($5, $2);}
       ;

exp : LEFT_PAR exp RIGHT_PAR { $$ = $2; }
    | exp OR exp { $$ = new_label(0, $1, $3); }
    | exp AND exp { $$ = new_label(1, $1, $3); }
    | NOT exp { $$ = new_label(2, $2); }
    | CASE_TRUE { $$ = new_label(4); }
    | ID { $$ = new_label(3, $1); }
    ;
 
%%



void yyerror(const char *s){
  fprintf (stderr, "%s\n", s);
}



