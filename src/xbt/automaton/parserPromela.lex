/* Copyright (c) 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

%option noyywrap

%{

#include "simgrid_config.h"
#if !HAVE_UNISTD_H
#define YY_NO_UNISTD_H /* hello Windows */

#ifdef _MSC_VER
# include <io.h>
# include <process.h>
# define _CRT_SECURE_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
#endif
#endif

#include <stdio.h>
#include "parserPromela.tab.hacc"
  
  extern YYSTYPE yylval;
 
%}

blancs       [ \t]+
espace       [ ]+
nouv_ligne   [ \n]

chiffre      [0-9]
entier       {chiffre}+
reel         {entier}("."{entier})
caractere    [a-zA-Z0-9_]

numl         \n

chaine       \"({caractere}*|\n|\\|\"|{espace}*)*\"

commentaire  "/*"([^\*\/]*{nouv_ligne}*[^\*\/]*)*"*/"

%%

"never"      { return (NEVER); }
"if"         { return (IF); }
"fi"         { return (FI); }
"->"         { return (IMPLIES); }
"goto"       { return (GOTO); }
"&&"         { return (AND); }
"||"         { return (OR); }
"!"          { return (NOT); }
"("          { return (LEFT_PAR); }
")"          { return (RIGHT_PAR); }
"::"         { return (CASE); }
":"          { return (COLON); }
";"          { return (SEMI_COLON); }
"1"          { return (CASE_TRUE); }
"{"          { return (LEFT_BRACE); }
"}"          { return (RIGHT_BRACE); }


{commentaire}             { }

{blancs}                  { }


{reel}                    { sscanf(yytext,"%lf",&yylval.real); 
                            return (LITT_REEL); }

{entier}                  { sscanf(yytext,"%d",&yylval.integer); 
                            return (LITT_ENT); }

{chaine}                  { yylval.string=(char *)malloc(strlen(yytext)+1);
                            sscanf(yytext,"%s",yylval.string); 
                            return (LITT_CHAINE); }

[a-zA-Z]{caractere}*      { yylval.string=(char *)malloc(strlen(yytext)+1);
                            sscanf(yytext,"%s",yylval.string);
			                      return (ID); }
		   
{numl}                    { }

.                         { }

%%


