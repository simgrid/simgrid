/* $Id$ */

/* gs/parse.h -- automatic parsing of data structures */

/* Authors: Arnaud Legrand, Martin Quinson            */

typedef enum {
  GRAS_DDT_PARSE_TOKEN_EMPTY = 0,
  GRAS_DDT_PARSE_TOKEN_LP = 512, /* { */
  GRAS_DDT_PARSE_TOKEN_RP,       /* } */
  GRAS_DDT_PARSE_TOKEN_LB,       /* [ */
  GRAS_DDT_PARSE_TOKEN_RB,       /* ] */
  GRAS_DDT_PARSE_TOKEN_WORD,
  GRAS_DDT_PARSE_TOKEN_SPACE,
  GRAS_DDT_PARSE_TOKEN_QUOTE,
  GRAS_DDT_PARSE_TOKEN_COMMENT,
  GRAS_DDT_PARSE_TOKEN_NEWLINE,
  GRAS_DDT_PARSE_TOKEN_STAR,
  GRAS_DDT_PARSE_TOKEN_SEMI_COLON,
  GRAS_DDT_PARSE_TOKEN_COLON,
  GRAS_DDT_PARSE_TOKEN_ERROR
} gras_ddt_parse_token_t;

#define GRAS_DDT_PARSE_MAX_STR_CONST 4048

extern int gras_ddt_parse_line_pos;
extern int gras_ddt_parse_col_pos;
extern int gras_ddt_parse_char_pos;
extern int gras_ddt_parse_tok_num;

void gras_ddt_parse_dump(void);
int gras_ddt_parse_lex(void);
int gras_ddt_parse_lex_n_dump(void);
void  gras_ddt_parse_pointer_init(const char *file);
void  gras_ddt_parse_pointer_close(void);
void  gras_ddt_parse_pointer_string_init(const char *string_to_parse);
void  gras_ddt_parse_pointer_string_close(void);
