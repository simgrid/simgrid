%option noyywrap

%{


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


