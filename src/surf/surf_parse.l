/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

%option noyywrap
%{
#include "xbt/sysdep.h"
#include"surf/surf_parse.h"
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(parse, surf ,"Logging specific to the SURF  module");

  YY_BUFFER_STATE surf_input_buffer;
  FILE *surf_file_to_parse;

  int surf_line_pos = 1;
  int surf_char_pos = -1;
  int surf_tok_num = 0;
  char string_buf[MAX_STR_CONST];
  char *string_buf_ptr = NULL;
%}

%x comment foo str
space           [ \t]
letter          [A-Za-z._-]
digit           [0-9]

%%
        int comment_caller=0;


"//"[^\n]*
"/*"         {
             comment_caller = INITIAL;
             BEGIN(comment);
             }

<foo>"/*"    {
             comment_caller = foo;
             BEGIN(comment);
             }

<comment>[^*\n]*        /* eat anything that's not a '*' */
<comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
<comment>\n             {++surf_line_pos;surf_char_pos=0;}
<comment>"*"+"/"        BEGIN(comment_caller);

\"      string_buf_ptr = string_buf; surf_char_pos++; BEGIN(str);

<str>\"        { /* saw closing quote - all done */
        BEGIN(INITIAL);
        *string_buf_ptr = '\0';
	surf_parse_text=string_buf;
	surf_char_pos++;
	return TOKEN_WORD;
        /* return string constant token type and
         * value to parser
         */
        }

<str>\n        {
        /* error - unterminated string constant */
        /* generate error message */
        }

<str>\\[0-7]{1,3} {
        /* octal escape sequence */
        int result;

        (void) sscanf( surf_parse_text + 1, "%o", &result );

        if ( result > 0xff )
                /* error, constant is out-of-bounds */

        *string_buf_ptr++ = result;
	surf_char_pos++;
        }

<str>\\[0-9]+ {
        /* generate error - bad escape sequence; something
         * like '\48' or '\0777777'
         */
        }

<str>\\n  {*string_buf_ptr++ = '\n';	surf_char_pos++;}
<str>\\t  {*string_buf_ptr++ = '\t';	surf_char_pos++;}
<str>\\r  {*string_buf_ptr++ = '\r';	surf_char_pos++;}
<str>\\b  {*string_buf_ptr++ = '\b';	surf_char_pos++;}
<str>\\f  {*string_buf_ptr++ = '\f'; 	surf_char_pos++;}

<str>\\(.|\n)  {*string_buf_ptr++ = surf_parse_text[1]; 	
                if(surf_parse_text[1]=='\n') {
		  ++surf_line_pos;surf_char_pos=0;
	        } else { surf_char_pos++;}
               }

<str>[^\\\n\"]+        {
        char *yptr = surf_parse_text;

        while ( *yptr )
	  *string_buf_ptr++ = *yptr++;
          surf_char_pos++;
        }

({letter}|{digit})*     { return(TOKEN_WORD);} /* surf_char_pos+= strlen(surf_parse_text);  */
"("                     { surf_char_pos++; return(TOKEN_LP);}
")"                     { surf_char_pos++;return(TOKEN_RP);}
"</"                    { surf_char_pos+=2; return(TOKEN_END_SECTION);}
"<"                     { surf_char_pos++; return(TOKEN_BEGIN_SECTION);}
">"                     { surf_char_pos++;return(TOKEN_CLOSURE);}
"\n"                    { surf_line_pos++; surf_char_pos=-1; return(TOKEN_NEWLINE);}
. { surf_char_pos++;}
%%
/* {space}+                { return(TOKEN_SPACE);} */

static void __print_val(void) {
  switch(surf_tok_num) {
  case TOKEN_LP      : {printf("TOKEN_LP ");break;}
  case TOKEN_RP      : {printf("TOKEN_RP ");break;}
  case TOKEN_BEGIN_SECTION : {printf("TOKEN_BEGIN_SECTION ");break;}
  case TOKEN_END_SECTION : {printf("TOKEN_END_SECTION ");break;}
  case TOKEN_CLOSURE : {printf("TOKEN_CLOSURE ");break;}
  case TOKEN_WORD    : {printf("TOKEN_WORD ");break;}
  case TOKEN_NEWLINE : {printf("TOKEN_NEWLINE\n");return;}
  case TOKEN_EMPTY : {printf("TOKEN_EMPTY\n");return;}
  default             : {printf("Unknown token %d\n", surf_tok_num);return;}
  }

  printf("-->%s<-- [line %d, pos %d]\n",surf_parse_text,surf_line_pos,surf_char_pos);

  return;
}

e_surf_token_t surf_parse(void) {
  surf_tok_num = surf_parse_lex();
  __print_val();
  surf_char_pos += strlen(surf_parse_text);
  return(surf_tok_num);
}

void find_section(const char* file, const char* section_name)
{
  e_surf_token_t token;
  int found = 0;

  surf_parse_open(file);

  while((token=surf_parse())) {
    if(token!=TOKEN_BEGIN_SECTION) continue;

    token=surf_parse();
    xbt_assert1((token==TOKEN_WORD),"Parse error line %d",surf_line_pos);
    if(strcmp(surf_parse_text,section_name)==0) found=1;

    token=surf_parse();
    xbt_assert1((token==TOKEN_CLOSURE),"Parse error line %d",surf_line_pos);

    if(found) return;
  }

  CRITICAL2("Could not find %s section in %s\n",section_name,file);
  xbt_abort();
}

void close_section(const char* section_name)
{
  e_surf_token_t token;

  token=surf_parse();
  xbt_assert1((token==TOKEN_WORD),"Parse error line %d",surf_line_pos);
  xbt_assert1((strcmp(surf_parse_text,section_name)==0), 
	      "Closing section does not match the opening one (%s).", 
	      section_name);
  
  token=surf_parse();
  xbt_assert1((token==TOKEN_CLOSURE),"Parse error line %d",surf_line_pos);

  surf_parse_close();
}

void  surf_parse_open(const char *file) {
  surf_file_to_parse = fopen(file,"r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n",file);
  surf_input_buffer = surf_parse__create_buffer( surf_file_to_parse, 10 );
  surf_parse__switch_to_buffer(surf_input_buffer);

  surf_line_pos = 1;
  surf_char_pos = 0;
  surf_tok_num = 0;
}

void  surf_parse_close(void) {
  surf_parse__delete_buffer(surf_input_buffer);
  fclose(surf_file_to_parse);

  surf_line_pos = 1;
  surf_char_pos = 0;
  surf_tok_num = 0;
}

void surf_parse_double(double *value)
{ 
  e_surf_token_t token;
  int ret = 0;

  token = surf_parse();		/* power_scale */
  xbt_assert1((token == TOKEN_WORD), "Parse error line %d", surf_line_pos);
  ret = sscanf(surf_parse_text, "%lg", value);
  xbt_assert2((ret==1), "Parse error line %d : %s not a number", surf_line_pos,
	      surf_parse_text);
}

void surf_parse_trace(tmgr_trace_t *trace)
{
  e_surf_token_t token;
  
  token = surf_parse();		/* power_trace */
  xbt_assert1((token == TOKEN_WORD), "Parse error line %d", surf_line_pos);
  if (strcmp(surf_parse_text, "") == 0)
    *trace = NULL;
  else
    *trace = tmgr_trace_new(surf_parse_text);
}

void surf_parse_deployment_line(char **host, int *argc, char ***argv)
{
  e_surf_token_t token;


  /* Parse Host name */
  *host = xbt_strdup(surf_parse_text);  

  *argc = 0;
  *argv = NULL;

  /* Parse command line */
  while((token = surf_parse())) {
    if(token == TOKEN_NEWLINE) return; 
    xbt_assert1((token == TOKEN_WORD), "Parse error line %d", surf_line_pos);

    (*argc)++;
    *argv=xbt_realloc (*argv, (*argc) * sizeof(char*));
    (*argv)[(*argc)-1]=xbt_strdup(surf_parse_text);
  }
}


/*  Local variables: */
/*  mode: c */
/*  End: */
