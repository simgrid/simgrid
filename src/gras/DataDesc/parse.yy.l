/**** MSG_LICENCE DO NOT REMOVE ****/

%option noyywrap
%{
#include"gs/gs_private.h"
#include"gs/parse.yy.h"
#include <string.h>
  YY_BUFFER_STATE input_buffer;
  FILE *file_to_parse;

  int gs_parse_line_pos = 1;
  int gs_parse_char_pos = 0;
  int gs_parse_tok_num = 0;
%}

%x comment foo str
space           [ \t]
letter          [A-Za-z._-]
digit           [0-9]

%%
        int comment_caller=0;

        char string_buf[GS_PARSE_MAX_STR_CONST];
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
<comment>\n             {++gs_parse_line_pos;gs_parse_char_pos=0;}
<comment>"*"+"/"        BEGIN(comment_caller);

\"      string_buf_ptr = string_buf; gs_parse_char_pos++; BEGIN(str);

<str>\"        { /* saw closing quote - all done */
        BEGIN(INITIAL);
        *string_buf_ptr = '\0';
	yytext=string_buf;
	gs_parse_char_pos++;
	return GS_PARSE_TOKEN_WORD;
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
	gs_parse_char_pos++;
        }

<str>\\[0-9]+ {
        /* generate error - bad escape sequence; something
         * like '\48' or '\0777777'
         */
        }

<str>\\n  {*string_buf_ptr++ = '\n';	gs_parse_char_pos++;}
<str>\\t  {*string_buf_ptr++ = '\t';	gs_parse_char_pos++;}
<str>\\r  {*string_buf_ptr++ = '\r';	gs_parse_char_pos++;}
<str>\\b  {*string_buf_ptr++ = '\b';	gs_parse_char_pos++;}
<str>\\f  {*string_buf_ptr++ = '\f'; 	gs_parse_char_pos++;}

<str>\\(.|\n)  {*string_buf_ptr++ = yytext[1]; 	
                if(yytext[1]=='\n') {
		  ++gs_parse_line_pos;gs_parse_char_pos=0;
	        } else { gs_parse_char_pos++;}
               }

<str>[^\\\n\"]+        {
        char *yptr = yytext;

        while ( *yptr )
	  *string_buf_ptr++ = *yptr++;
          gs_parse_char_pos++;
        }

({letter}|{digit})*     { gs_parse_char_pos+= strlen(yytext); return(GS_PARSE_TOKEN_WORD);}
"{"                     { gs_parse_char_pos++; return(GS_PARSE_TOKEN_LP);}
"}"                     { gs_parse_char_pos++;return(GS_PARSE_TOKEN_RP);}
"*"                     { gs_parse_char_pos++;return(GS_PARSE_TOKEN_STAR);}
";"                     { gs_parse_char_pos++;return(GS_PARSE_TOKEN_SEMI_COLON);}
","                     { gs_parse_char_pos++;return(GS_PARSE_TOKEN_COLON);}
"\n"                    { gs_parse_line_pos++; gs_parse_char_pos=0;}
. { gs_parse_char_pos++;}
%%
/* {space}+                { return(TOKEN_SPACE);} */

void gs_parse_dump(void) {
  switch(gs_parse_tok_num) {
  case GS_PARSE_TOKEN_LP      : {printf("TOKEN_LP ");break;}
  case GS_PARSE_TOKEN_RP      : {printf("TOKEN_RP ");break;}
  case GS_PARSE_TOKEN_WORD    : {printf("TOKEN_WORD ");break;}
    //  case GS_PARSE_TOKEN_SPACE   : {printf("TOKEN_SPACE ");break;}
    //  case GS_PARSE_TOKEN_COMMENT : {printf("TOKEN_COMMENT ");break;}
  case GS_PARSE_TOKEN_NEWLINE : {printf("TOKEN_NEWLINE\n");return;}
  case GS_PARSE_TOKEN_EMPTY : {printf("TOKEN_EMPTY\n");return;}
  default             : {printf("Unknown token %d\n", gs_parse_tok_num);return;}
  }
  printf("-->%s<-- [line %d, pos %d]\n",yytext,gs_parse_line_pos,gs_parse_char_pos);
  return;
}

int gs_parse_lex_n_dump(void) {
  gs_parse_tok_num = gs_parse_lex();
  //  voir_val();
  //  gs_parse_char_pos += strlen(yytext);
  return(gs_parse_tok_num);
}

void  gs_parse_pointer_init(const char *file) {
  file_to_parse = fopen(file,"r");
  input_buffer = yy_create_buffer( file_to_parse, 10 );
  yy_switch_to_buffer(input_buffer);

  gs_parse_line_pos = 1;
  gs_parse_char_pos = 0;
  gs_parse_tok_num = 0;
}

void  gs_parse_pointer_close(void) {
  yy_delete_buffer(input_buffer);
  fclose(file_to_parse);

  gs_parse_line_pos = 1;
  gs_parse_char_pos = 0;
  gs_parse_tok_num = 0;
}


void  gs_parse_pointer_string_init(const char *string_to_parse) {
  input_buffer = yy_scan_string (string_to_parse);
  yy_switch_to_buffer(input_buffer);

  gs_parse_line_pos = 1;
  gs_parse_char_pos = 0;
  gs_parse_tok_num = 0;
}

void  gs_parse_pointer_string_close(void) {
  yy_delete_buffer(input_buffer);

  gs_parse_line_pos = 1;
  gs_parse_char_pos = 0;
  gs_parse_tok_num = 0;
}

// Local variables:
// mode: c
// End:
