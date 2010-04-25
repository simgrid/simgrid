/* DataDesc/ddt_parse -- automatic parsing of data structures */

/* Copyright (c) 2004 Arnaud Legrand, Martin Quinson. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

%option noyywrap
%{
#include "gras/DataDesc/datadesc_private.h"
#include "gras/DataDesc/ddt_parse.yy.h"
#include <string.h>
  YY_BUFFER_STATE gras_ddt_input_buffer;
  FILE *gras_ddt_file_to_parse;

  int gras_ddt_parse_line_pos = 1;
  int gras_ddt_parse_col_pos = 0;
  int gras_ddt_parse_char_pos = 0;
  int gras_ddt_parse_tok_num = 0;
  const char *definition;
  XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_ddt_lexer,gras_ddt_parse,"The crude internals of the lexer used for type parsing");
#define SHOW_WHERE DEBUG4("%d:%d (char #%d): seen '%s'", gras_ddt_parse_line_pos,gras_ddt_parse_col_pos,gras_ddt_parse_char_pos,yytext)
%}

%x annotate comment foo
space           [ \t]
letter          [A-Za-z._-]
digit           [0-9]

%%
   int comment_caller=0;
   int annotate_caller=0;

"//"[^\n]*

"/*g"{space}* { /****************** ANNOTATION ************************/
  DEBUG0("Begin annotation");
  annotate_caller = INITIAL;
  gras_ddt_parse_char_pos+= strlen(yytext);
  gras_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(annotate);
}
<foo>"/*g"{space}* { /* trim annotation */
  DEBUG0("Begin annotation");
  annotate_caller = foo;
  gras_ddt_parse_char_pos+= strlen(yytext);
  gras_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(annotate);
}

<annotate>{space}*"g*/" {
  DEBUG0("End annotation");
  gras_ddt_parse_char_pos+= strlen(yytext);
  gras_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(annotate_caller);
}

<annotate>"*/" {
  PARSE_ERROR0("``/*g'' construct closed by a regular ``*/''");
}
<annotate>\n  {
  PARSE_ERROR0("Type annotation cannot spread over several lines");
}

<annotate>.* { /* eat the rest */
  gras_ddt_parse_char_pos+= strlen(yytext);
  gras_ddt_parse_col_pos+= strlen(yytext);
  return GRAS_DDT_PARSE_TOKEN_ANNOTATE;
}

"/*[^g]"   { /****************** COMMENTS ************************/
  /* constructs like : */
    /*g [string] g*/ 
  /* are not comments but size annotations */
  comment_caller = INITIAL;
  BEGIN(comment);
}

<foo>"/*[^g]"    {
  comment_caller = foo;
  BEGIN(comment);
}

<comment>[^*\n]*      { /* eat anything that's not a '*' */
}
<comment>"*"+[^*/\n]* { /* eat up '*'s not followed by '/'s */
}
<comment>\n             {
  ++gras_ddt_parse_line_pos;
  gras_ddt_parse_col_pos=0;
  gras_ddt_parse_char_pos++;
}
<comment>"*"+"/" {
  gras_ddt_parse_char_pos+= strlen(yytext);
  gras_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(comment_caller);
}

({letter}|{digit})* {  /****************** STATEMENTS ************************/
  gras_ddt_parse_char_pos += strlen(yytext);
  gras_ddt_parse_col_pos += strlen(yytext);
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_WORD);
}
"{"  { 
  gras_ddt_parse_char_pos++; 
  gras_ddt_parse_col_pos++; 
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_LA);
}
"}" {
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_RA);
}
"["  { 
  gras_ddt_parse_char_pos++; 
  gras_ddt_parse_col_pos++; 
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_LB);
}
"]" {
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_RB);
}
"("  { 
  gras_ddt_parse_char_pos++; 
  gras_ddt_parse_col_pos++; 
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_LP);
}
")" {
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_RP);
}
"*" {
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_STAR);
}
";" {
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_SEMI_COLON);
}
"," { 
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(GRAS_DDT_PARSE_TOKEN_COLON);
}
"\n" {
 gras_ddt_parse_line_pos++; 
 gras_ddt_parse_char_pos++;
 gras_ddt_parse_col_pos=0;
  SHOW_WHERE;
}
. { 
  gras_ddt_parse_char_pos++;
  gras_ddt_parse_col_pos++;
  SHOW_WHERE;
}
%%
/* {space}+                { return(TOKEN_SPACE);} */

void gras_ddt_parse_dump(void) {
  switch(gras_ddt_parse_tok_num) {
  case GRAS_DDT_PARSE_TOKEN_LA      : {printf("TOKEN_LA ");break;}
  case GRAS_DDT_PARSE_TOKEN_RA      : {printf("TOKEN_RA ");break;}
  case GRAS_DDT_PARSE_TOKEN_WORD    : {printf("TOKEN_WORD ");break;}
    /*  case GRAS_DDT_PARSE_TOKEN_SPACE   : {printf("TOKEN_SPACE ");break;}*/
    /*  case GRAS_DDT_PARSE_TOKEN_COMMENT : {printf("TOKEN_COMMENT ");break;}*/
  case GRAS_DDT_PARSE_TOKEN_NEWLINE : {printf("TOKEN_NEWLINE\n");return;}
  case GRAS_DDT_PARSE_TOKEN_EMPTY : {printf("TOKEN_EMPTY\n");return;}
  default             : {printf("Unknown token %d\n", gras_ddt_parse_tok_num);return;}
  }
  printf("-->%s<-- [line %d, pos %d]\n",yytext,gras_ddt_parse_line_pos,gras_ddt_parse_char_pos);
  return;
}

int gras_ddt_parse_lex_n_dump(void) {
  gras_ddt_parse_tok_num = gras_ddt_parse_lex();
  /*  gras_ddt_parse_char_pos += strlen(yytext);*/
  return(gras_ddt_parse_tok_num);
}

void  gras_ddt_parse_pointer_init(const char *file) {
  gras_ddt_file_to_parse = fopen(file,"r");
  gras_ddt_input_buffer = yy_create_buffer( gras_ddt_file_to_parse, 10 );
  yy_switch_to_buffer(gras_ddt_input_buffer);

  gras_ddt_parse_line_pos = 1;
  gras_ddt_parse_char_pos = 0;
  gras_ddt_parse_col_pos = 0;
  gras_ddt_parse_tok_num = 0;
}

void  gras_ddt_parse_pointer_close(void) {
  yy_delete_buffer(gras_ddt_input_buffer);
  fclose(gras_ddt_file_to_parse);

  gras_ddt_parse_line_pos = 1;
  gras_ddt_parse_char_pos = 0;
  gras_ddt_parse_tok_num = 0;
}


void  gras_ddt_parse_pointer_string_init(const char *string_to_parse) {
  gras_ddt_input_buffer = yy_scan_string (string_to_parse);
  definition = string_to_parse;
  yy_switch_to_buffer(gras_ddt_input_buffer);

  gras_ddt_parse_line_pos = 1;
  gras_ddt_parse_char_pos = 0;
  gras_ddt_parse_tok_num = 0;
}

void  gras_ddt_parse_pointer_string_close(void) {
  yy_delete_buffer(gras_ddt_input_buffer);

  gras_ddt_parse_line_pos = 1;
  gras_ddt_parse_char_pos = 0;
  gras_ddt_parse_tok_num = 0;

  if (0)
    yyunput('\0',NULL); /* fake a use of this function to calm gcc down */
}

/* Local variables:*/
/* mode: c */
/* End: */
