/**** MSG_LICENCE DO NOT REMOVE ****/

%option noyywrap
%{
#include"DataDesc/gs_private.h"
#include"DataDesc/parse.yy.h"
#include <string.h>
  YY_BUFFER_STATE input_buffer;
  FILE *file_to_parse;

  int gras_datadesc_parse_line_pos = 1;
  int gras_datadesc_parse_char_pos = 0;
  int gras_datadesc_parse_tok_num = 0;
%}

%x comment foo str
space           [ \t]
letter          [A-Za-z._-]
digit           [0-9]

%%
        int comment_caller=0;

        char string_buf[GRAS_DATADESC_PARSE_MAX_STR_CONST];
        char *string_buf_ptr = NULL;

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
<comment>\n             {++gras_datadesc_parse_line_pos;gras_datadesc_parse_char_pos=0;}
<comment>"*"+"/"        BEGIN(comment_caller);

\"      string_buf_ptr = string_buf; gras_datadesc_parse_char_pos++; BEGIN(str);

<str>\"        { /* saw closing quote - all done */
        BEGIN(INITIAL);
        *string_buf_ptr = '\0';
	yytext=string_buf;
	gras_datadesc_parse_char_pos++;
	return GRAS_DATADESC_PARSE_TOKEN_WORD;
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

        (void) sscanf( yytext + 1, "%o", &result );

        if ( result > 0xff )
                /* error, constant is out-of-bounds */

        *string_buf_ptr++ = result;
	gras_datadesc_parse_char_pos++;
        }

<str>\\[0-9]+ {
        /* generate error - bad escape sequence; something
         * like '\48' or '\0777777'
         */
        }

<str>\\n  {*string_buf_ptr++ = '\n';	gras_datadesc_parse_char_pos++;}
<str>\\t  {*string_buf_ptr++ = '\t';	gras_datadesc_parse_char_pos++;}
<str>\\r  {*string_buf_ptr++ = '\r';	gras_datadesc_parse_char_pos++;}
<str>\\b  {*string_buf_ptr++ = '\b';	gras_datadesc_parse_char_pos++;}
<str>\\f  {*string_buf_ptr++ = '\f'; 	gras_datadesc_parse_char_pos++;}

<str>\\(.|\n)  {*string_buf_ptr++ = yytext[1]; 	
                if(yytext[1]=='\n') {
		  ++gras_datadesc_parse_line_pos;gras_datadesc_parse_char_pos=0;
	        } else { gras_datadesc_parse_char_pos++;}
               }

<str>[^\\\n\"]+        {
        char *yptr = yytext;

        while ( *yptr )
	  *string_buf_ptr++ = *yptr++;
          gras_datadesc_parse_char_pos++;
        }

({letter}|{digit})*     { gras_datadesc_parse_char_pos+= strlen(yytext); return(GRAS_DATADESC_PARSE_TOKEN_WORD);}
"{"                     { gras_datadesc_parse_char_pos++; return(GRAS_DATADESC_PARSE_TOKEN_LP);}
"}"                     { gras_datadesc_parse_char_pos++;return(GRAS_DATADESC_PARSE_TOKEN_RP);}
"*"                     { gras_datadesc_parse_char_pos++;return(GRAS_DATADESC_PARSE_TOKEN_STAR);}
";"                     { gras_datadesc_parse_char_pos++;return(GRAS_DATADESC_PARSE_TOKEN_SEMI_COLON);}
","                     { gras_datadesc_parse_char_pos++;return(GRAS_DATADESC_PARSE_TOKEN_COLON);}
"\n"                    { gras_datadesc_parse_line_pos++; gras_datadesc_parse_char_pos=0;}
. { gras_datadesc_parse_char_pos++;}
%%
/* {space}+                { return(TOKEN_SPACE);} */

void gras_datadesc_parse_dump(void) {
  switch(gras_datadesc_parse_tok_num) {
  case GRAS_DATADESC_PARSE_TOKEN_LP      : {printf("TOKEN_LP ");break;}
  case GRAS_DATADESC_PARSE_TOKEN_RP      : {printf("TOKEN_RP ");break;}
  case GRAS_DATADESC_PARSE_TOKEN_WORD    : {printf("TOKEN_WORD ");break;}
    //  case GRAS_DATADESC_PARSE_TOKEN_SPACE   : {printf("TOKEN_SPACE ");break;}
    //  case GRAS_DATADESC_PARSE_TOKEN_COMMENT : {printf("TOKEN_COMMENT ");break;}
  case GRAS_DATADESC_PARSE_TOKEN_NEWLINE : {printf("TOKEN_NEWLINE\n");return;}
  case GRAS_DATADESC_PARSE_TOKEN_EMPTY : {printf("TOKEN_EMPTY\n");return;}
  default             : {printf("Unknown token %d\n", gras_datadesc_parse_tok_num);return;}
  }
  printf("-->%s<-- [line %d, pos %d]\n",yytext,gras_datadesc_parse_line_pos,gras_datadesc_parse_char_pos);
  return;
}

int gras_datadesc_parse_lex_n_dump(void) {
  gras_datadesc_parse_tok_num = gras_datadesc_parse_lex();
  //  voir_val();
  //  gras_datadesc_parse_char_pos += strlen(yytext);
  return(gras_datadesc_parse_tok_num);
}

void  gras_datadesc_parse_pointer_init(const char *file) {
  file_to_parse = fopen(file,"r");
  input_buffer = yy_create_buffer( file_to_parse, 10 );
  yy_switch_to_buffer(input_buffer);

  gras_datadesc_parse_line_pos = 1;
  gras_datadesc_parse_char_pos = 0;
  gras_datadesc_parse_tok_num = 0;
}

void  gras_datadesc_parse_pointer_close(void) {
  yy_delete_buffer(input_buffer);
  fclose(file_to_parse);

  gras_datadesc_parse_line_pos = 1;
  gras_datadesc_parse_char_pos = 0;
  gras_datadesc_parse_tok_num = 0;
}


void  gras_datadesc_parse_pointer_string_init(const char *string_to_parse) {
  input_buffer = yy_scan_string (string_to_parse);
  yy_switch_to_buffer(input_buffer);

  gras_datadesc_parse_line_pos = 1;
  gras_datadesc_parse_char_pos = 0;
  gras_datadesc_parse_tok_num = 0;
}

void  gras_datadesc_parse_pointer_string_close(void) {
  yy_delete_buffer(input_buffer);

  gras_datadesc_parse_line_pos = 1;
  gras_datadesc_parse_char_pos = 0;
  gras_datadesc_parse_tok_num = 0;
}

// Local variables:
// mode: c
// End:
