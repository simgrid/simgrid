/* ddt_parse.h -- automatic parsing of data structures                      */

/* Copyright (c) 2004, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

typedef enum {
  XBT_DDT_PARSE_TOKEN_EMPTY = 0,
  XBT_DDT_PARSE_TOKEN_LA = 512,        /* { 'A' for the french "accolade" since there is a name conflict in english (braket/brace) */
  XBT_DDT_PARSE_TOKEN_RA,      /* } */
  XBT_DDT_PARSE_TOKEN_LB,      /* [ */
  XBT_DDT_PARSE_TOKEN_RB,      /* ] */
  XBT_DDT_PARSE_TOKEN_LP,      /* ( */
  XBT_DDT_PARSE_TOKEN_RP,      /* ) */
  XBT_DDT_PARSE_TOKEN_WORD,
  XBT_DDT_PARSE_TOKEN_SPACE,
  XBT_DDT_PARSE_TOKEN_COMMENT,
  XBT_DDT_PARSE_TOKEN_ANNOTATE,
  XBT_DDT_PARSE_TOKEN_NEWLINE,
  XBT_DDT_PARSE_TOKEN_STAR,
  XBT_DDT_PARSE_TOKEN_SEMI_COLON,
  XBT_DDT_PARSE_TOKEN_COLON,   /* impossible since the macro think that it's a arg separator. 
                                   But handle anyway for the *vicious* calling xbt_ddt_parse manually */
  XBT_DDT_PARSE_TOKEN_ERROR
} xbt_ddt_parse_token_t;

#define XBT_DDT_PARSE_MAX_STR_CONST 4048

extern int xbt_ddt_parse_line_pos;
extern int xbt_ddt_parse_col_pos;
extern int xbt_ddt_parse_char_pos;
extern int xbt_ddt_parse_tok_num;

void xbt_ddt_parse_dump(void);
int xbt_ddt_parse_lex_n_dump(void);
void xbt_ddt_parse_pointer_init(const char *file);
void xbt_ddt_parse_pointer_close(void);
void xbt_ddt_parse_pointer_string_init(const char *string_to_parse);
void xbt_ddt_parse_pointer_string_close(void);

/* prototypes of the functions offered by flex */
int xbt_ddt_parse_lex(void);
int xbt_ddt_parse_get_lineno(void);
FILE *xbt_ddt_parse_get_in(void);
FILE *xbt_ddt_parse_get_out(void);
int xbt_ddt_parse_get_leng(void);
char *xbt_ddt_parse_get_text(void);
void xbt_ddt_parse_set_lineno(int line_number);
void xbt_ddt_parse_set_in(FILE * in_str);
void xbt_ddt_parse_set_out(FILE * out_str);
int xbt_ddt_parse_get_debug(void);
void xbt_ddt_parse_set_debug(int bdebug);
int xbt_ddt_parse_lex_destroy(void);

#define PARSE_ERROR(...)                                                \
  PARSE_ERROR_(__VA_ARGS__,                                             \
               xbt_ddt_parse_line_pos, xbt_ddt_parse_col_pos, definition)
#define PARSE_ERROR_(fmt, ...)                           \
  do {                                                   \
    XBT_ERROR(fmt " at %d:%d of :\n%s", __VA_ARGS__);   \
    xbt_abort();                                         \
  } while (0)
