/* ddt_parse.h -- automatic parsing of data structures                      */

/* Copyright (c) 2004, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

typedef enum {
  GRAS_DDT_PARSE_TOKEN_EMPTY = 0,
  GRAS_DDT_PARSE_TOKEN_LA = 512,        /* { 'A' for the french "accolade" since there is a name conflict in english (braket/brace) */
  GRAS_DDT_PARSE_TOKEN_RA,      /* } */
  GRAS_DDT_PARSE_TOKEN_LB,      /* [ */
  GRAS_DDT_PARSE_TOKEN_RB,      /* ] */
  GRAS_DDT_PARSE_TOKEN_LP,      /* ( */
  GRAS_DDT_PARSE_TOKEN_RP,      /* ) */
  GRAS_DDT_PARSE_TOKEN_WORD,
  GRAS_DDT_PARSE_TOKEN_SPACE,
  GRAS_DDT_PARSE_TOKEN_COMMENT,
  GRAS_DDT_PARSE_TOKEN_ANNOTATE,
  GRAS_DDT_PARSE_TOKEN_NEWLINE,
  GRAS_DDT_PARSE_TOKEN_STAR,
  GRAS_DDT_PARSE_TOKEN_SEMI_COLON,
  GRAS_DDT_PARSE_TOKEN_COLON,   /* impossible since the macro think that it's a arg separator. 
                                   But handle anyway for the *vicious* calling gras_ddt_parse manually */
  GRAS_DDT_PARSE_TOKEN_ERROR
} gras_ddt_parse_token_t;

#define GRAS_DDT_PARSE_MAX_STR_CONST 4048

extern int gras_ddt_parse_line_pos;
extern int gras_ddt_parse_col_pos;
extern int gras_ddt_parse_char_pos;
extern int gras_ddt_parse_tok_num;

void gras_ddt_parse_dump(void);
int gras_ddt_parse_lex_n_dump(void);
void gras_ddt_parse_pointer_init(const char *file);
void gras_ddt_parse_pointer_close(void);
void gras_ddt_parse_pointer_string_init(const char *string_to_parse);
void gras_ddt_parse_pointer_string_close(void);

/* prototypes of the functions offered by flex */
int gras_ddt_parse_lex(void);
int gras_ddt_parse_get_lineno(void);
FILE *gras_ddt_parse_get_in(void);
FILE *gras_ddt_parse_get_out(void);
int gras_ddt_parse_get_leng(void);
char *gras_ddt_parse_get_text(void);
void gras_ddt_parse_set_lineno(int line_number);
void gras_ddt_parse_set_in(FILE * in_str);
void gras_ddt_parse_set_out(FILE * out_str);
int gras_ddt_parse_get_debug(void);
void gras_ddt_parse_set_debug(int bdebug);
int gras_ddt_parse_lex_destroy(void);

#define PARSE_ERROR(...)                                                \
  PARSE_ERROR_(__VA_ARGS__,                                             \
               gras_ddt_parse_line_pos, gras_ddt_parse_col_pos, definition)
#define PARSE_ERROR_(fmt, ...)                           \
  do {                                                   \
    XBT_ERROR(fmt " at %d:%d of :\n%s", __VA_ARGS__);   \
    xbt_abort();                                         \
  } while (0)
