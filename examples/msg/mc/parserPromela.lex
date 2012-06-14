%option noyywrap

%{


#include <stdio.h>
#include "y.tab.h"
  
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

"never"      { printf("%s", yytext); return (NEVER); }
"if"         { printf("%s", yytext); return (IF); }
"fi"         { printf("%s", yytext); 
               return (FI); }
"->"         { printf("%s", yytext); return (IMPLIES); }
"goto"       { printf("%s", yytext); return (GOTO); }
"&&"         { printf("%s", yytext); return (AND); }
"||"         { printf("%s", yytext); return (OR); }
"!"          { printf("%s", yytext); return (NOT); }
"("          { printf("%s", yytext); return (LEFT_PAR); }
")"          { printf("%s", yytext); return (RIGHT_PAR); }
"::"         { printf("%s", yytext); return (CASE); }
":"          { printf("%s", yytext); return (COLON); }
";"          { printf("%s", yytext); return (SEMI_COLON); }
"1"          { printf("%s", yytext); return (CASE_TRUE); }
"{"          { printf("%s", yytext); return (LEFT_BRACE); }
"}"          { printf("%s", yytext); return (RIGHT_BRACE); }


{commentaire}             { printf(" ");}

{blancs}                  { printf("%s",yytext); }


{reel}                    { printf("%s",yytext); 
                            sscanf(yytext,"%lf",&yylval.real); 
                            return (LITT_REEL); }

{entier}                  { printf("%s",yytext); 
                            sscanf(yytext,"%d",&yylval.integer); 
                            return (LITT_ENT); }

{chaine}                  { printf("%s",yytext);  
                            yylval.string=(char *)malloc(strlen(yytext)+1);
                            sscanf(yytext,"%s",yylval.string); 
                            return (LITT_CHAINE); }

[a-zA-Z]{caractere}*      { printf("%s",yytext); 
			    yylval.string=(char *)malloc(strlen(yytext)+1);
			    sscanf(yytext,"%s",yylval.string);
			    return (ID); }
		   
{numl}                    { printf("\n"); }

.                         { printf("caractère inconnu\n"); }

%%


