/* $Id$ */

/* gs/parse.h -- automatic parsing of data structures */

/* Authors: Arnaud Legrand, Martin Quinson            */

typedef enum {
  GS_PARSE_TOKEN_EMPTY = 0,
  GS_PARSE_TOKEN_LP = 512,
  GS_PARSE_TOKEN_RP,
  GS_PARSE_TOKEN_WORD,
  GS_PARSE_TOKEN_SPACE,
  GS_PARSE_TOKEN_QUOTE,
  GS_PARSE_TOKEN_COMMENT,
  GS_PARSE_TOKEN_NEWLINE,
  GS_PARSE_TOKEN_STAR,
  GS_PARSE_TOKEN_SEMI_COLON,
  GS_PARSE_TOKEN_COLON,
  GS_PARSE_TOKEN_ERROR
} gs_parse_token_t;

#define GS_PARSE_MAX_STR_CONST 4048

extern int gs_parse_line_pos;
extern int gs_parse_char_pos;
extern int gs_parse_tok_num;

void gs_parse_dump(void);
int gs_parse_lex(void);
int gs_parse_lex_n_dump(void);
void  gs_parse_pointer_init(const char *file);
void  gs_parse_pointer_close(void);
void  gs_parse_pointer_string_init(const char *string_to_parse);
void  gs_parse_pointer_string_close(void);
