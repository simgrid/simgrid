/* DataDesc/ddt_parse -- automatic parsing of data structures */

/* Copyright (c) 2004 Arnaud Legrand, Martin Quinson. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

%option noyywrap
%{
#include "xbt/DataDesc/datadesc_private.h"
#include "xbt/DataDesc/ddt_parse.yy.h"
#include <string.h>
  YY_BUFFER_STATE xbt_ddt_input_buffer;
  FILE *xbt_ddt_file_to_parse;

  int xbt_ddt_parse_line_pos = 1;
  int xbt_ddt_parse_col_pos = 0;
  int xbt_ddt_parse_char_pos = 0;
  int xbt_ddt_parse_tok_num = 0;
  const char *definition;
  XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ddt_lexer,xbt_ddt_parse,"The crude internals of the lexer used for type parsing");
#define SHOW_WHERE XBT_DEBUG("%d:%d (char #%d): seen '%s'", xbt_ddt_parse_line_pos,xbt_ddt_parse_col_pos,xbt_ddt_parse_char_pos,yytext)
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
  XBT_DEBUG("Begin annotation");
  annotate_caller = INITIAL;
  xbt_ddt_parse_char_pos+= strlen(yytext);
  xbt_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(annotate);
}
<foo>"/*g"{space}* { /* trim annotation */
  XBT_DEBUG("Begin annotation");
  annotate_caller = foo;
  xbt_ddt_parse_char_pos+= strlen(yytext);
  xbt_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(annotate);
}

<annotate>{space}*"g*/" {
  XBT_DEBUG("End annotation");
  xbt_ddt_parse_char_pos+= strlen(yytext);
  xbt_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(annotate_caller);
}

<annotate>"*/" {
  PARSE_ERROR("``/*g'' construct closed by a regular ``*/''");
}
<annotate>\n  {
  PARSE_ERROR("Type annotation cannot spread over several lines");
}

<annotate>.* { /* eat the rest */
  xbt_ddt_parse_char_pos+= strlen(yytext);
  xbt_ddt_parse_col_pos+= strlen(yytext);
  return XBT_DDT_PARSE_TOKEN_ANNOTATE;
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
  ++xbt_ddt_parse_line_pos;
  xbt_ddt_parse_col_pos=0;
  xbt_ddt_parse_char_pos++;
}
<comment>"*"+"/" {
  xbt_ddt_parse_char_pos+= strlen(yytext);
  xbt_ddt_parse_col_pos+= strlen(yytext);
  BEGIN(comment_caller);
}

({letter}|{digit})* {  /****************** STATEMENTS ************************/
  xbt_ddt_parse_char_pos += strlen(yytext);
  xbt_ddt_parse_col_pos += strlen(yytext);
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_WORD);
}
"{"  { 
  xbt_ddt_parse_char_pos++; 
  xbt_ddt_parse_col_pos++; 
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_LA);
}
"}" {
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_RA);
}
"["  { 
  xbt_ddt_parse_char_pos++; 
  xbt_ddt_parse_col_pos++; 
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_LB);
}
"]" {
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_RB);
}
"("  { 
  xbt_ddt_parse_char_pos++; 
  xbt_ddt_parse_col_pos++; 
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_LP);
}
")" {
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_RP);
}
"*" {
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_STAR);
}
";" {
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_SEMI_COLON);
}
"," { 
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
  return(XBT_DDT_PARSE_TOKEN_COLON);
}
"\n" {
 xbt_ddt_parse_line_pos++; 
 xbt_ddt_parse_char_pos++;
 xbt_ddt_parse_col_pos=0;
  SHOW_WHERE;
}
. { 
  xbt_ddt_parse_char_pos++;
  xbt_ddt_parse_col_pos++;
  SHOW_WHERE;
}
%%
/* {space}+                { return(TOKEN_SPACE);} */

void xbt_ddt_parse_dump(void) {
  switch(xbt_ddt_parse_tok_num) {
  case XBT_DDT_PARSE_TOKEN_LA      : {printf("TOKEN_LA ");break;}
  case XBT_DDT_PARSE_TOKEN_RA      : {printf("TOKEN_RA ");break;}
  case XBT_DDT_PARSE_TOKEN_WORD    : {printf("TOKEN_WORD ");break;}
    /*  case XBT_DDT_PARSE_TOKEN_SPACE   : {printf("TOKEN_SPACE ");break;}*/
    /*  case XBT_DDT_PARSE_TOKEN_COMMENT : {printf("TOKEN_COMMENT ");break;}*/
  case XBT_DDT_PARSE_TOKEN_NEWLINE : {printf("TOKEN_NEWLINE\n");return;}
  case XBT_DDT_PARSE_TOKEN_EMPTY : {printf("TOKEN_EMPTY\n");return;}
  default             : {printf("Unknown token %d\n", xbt_ddt_parse_tok_num);return;}
  }
  printf("-->%s<-- [line %d, pos %d]\n",yytext,xbt_ddt_parse_line_pos,xbt_ddt_parse_char_pos);
  return;
}

int xbt_ddt_parse_lex_n_dump(void) {
  xbt_ddt_parse_tok_num = xbt_ddt_parse_lex();
  /*  xbt_ddt_parse_char_pos += strlen(yytext);*/
  return(xbt_ddt_parse_tok_num);
}

void  xbt_ddt_parse_pointer_init(const char *file) {
  xbt_ddt_file_to_parse = fopen(file,"r");
  xbt_ddt_input_buffer = yy_create_buffer( xbt_ddt_file_to_parse, 10 );
  yy_switch_to_buffer(xbt_ddt_input_buffer);

  xbt_ddt_parse_line_pos = 1;
  xbt_ddt_parse_char_pos = 0;
  xbt_ddt_parse_col_pos = 0;
  xbt_ddt_parse_tok_num = 0;
}

void  xbt_ddt_parse_pointer_close(void) {
  yy_delete_buffer(xbt_ddt_input_buffer);
  fclose(xbt_ddt_file_to_parse);

  xbt_ddt_parse_line_pos = 1;
  xbt_ddt_parse_char_pos = 0;
  xbt_ddt_parse_tok_num = 0;
}


void  xbt_ddt_parse_pointer_string_init(const char *string_to_parse) {
  xbt_ddt_input_buffer = yy_scan_string (string_to_parse);
  definition = string_to_parse;
  yy_switch_to_buffer(xbt_ddt_input_buffer);

  xbt_ddt_parse_line_pos = 1;
  xbt_ddt_parse_char_pos = 0;
  xbt_ddt_parse_tok_num = 0;
}

void  xbt_ddt_parse_pointer_string_close(void) {
  yy_delete_buffer(xbt_ddt_input_buffer);

  xbt_ddt_parse_line_pos = 1;
  xbt_ddt_parse_char_pos = 0;
  xbt_ddt_parse_tok_num = 0;

  if (0)
    yyunput('\0',NULL); /* fake a use of this function to calm gcc down */
}

/* Local variables:*/
/* mode: c */
/* End: */
